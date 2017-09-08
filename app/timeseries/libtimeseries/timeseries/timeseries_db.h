#ifndef TIMESERIES_TIMESERIES_DB_H_
#define TIMESERIES_TIMESERIES_DB_H_

#include <thread>

#include "monolog.h"
#include "tiered_index.h"
#include "server/timeseries_db_types.h"

using namespace ::utils;

namespace timeseries {

typedef int64_t data_ptr_t;

struct data_pt {
  timestamp_t timestamp;
  value_t value;
};

struct stats {
  version_t version;
  value_t min;
  value_t max;
  value_t sum;
  value_t count;
  atomic::type<stats*> prv;

  stats()
      : version(std::numeric_limits<value_t>::max()),
        min(std::numeric_limits<value_t>::max()),
        max(0),
        sum(0),
        count(0) {
    atomic::init(&prv, static_cast<stats*>(nullptr));
  }

  stats(const stats& other) {
    version = other.version;
    min = other.min;
    max = other.max;
    sum = other.sum;
    count = other.count;
    atomic::init(&prv, atomic::load(&other.prv));
  }

  stats(version_t ver, value_t mn, value_t mx, value_t sm, value_t cnt)
      : version(ver),
        min(mn),
        max(mx),
        sum(sm),
        count(cnt) {
    atomic::init(&prv, static_cast<stats*>(nullptr));
  }

  stats(version_t ver, const data_pt* pt, size_t len)
      : version(ver),
        count(len) {
    min = std::numeric_limits<value_t>::max();
    max = 0;
    sum = 0;
    for (size_t i = 0; i < len; i++) {
      min = std::min(min, pt[i].value);
      max = std::max(max, pt[i].value);
      sum += pt[i].value;
    }
    atomic::init(&prv, static_cast<stats*>(nullptr));
  }

  stats(stats* p, version_t ver, const data_pt* pt, size_t len)
      : version(ver),
        count(len + p->count) {
    min = p->min;
    max = p->max;
    sum = p->sum;
    for (size_t i = 0; i < len; i++) {
      min = std::min(min, pt[i].value);
      max = std::max(max, pt[i].value);
      sum += pt[i].value;
    }
    atomic::init(&prv, p);
  }

  void reset() {
    min = std::numeric_limits<value_t>::max();
    max = 0;
    sum = 0;
    count = 0;
  }
};

// Single reader multi-writer timeseries
template<size_t branch_factor = 256, size_t depth = 4>
class timeseries_base {
 public:
  typedef monolog::mmap_monolog_relaxed<data_pt, 1024> data_log;
  typedef monolog::monolog_relaxed_linear<data_ptr_t, 32, 1024> ref_log;
  typedef datastore::index::tiered_index<ref_log, branch_factor, depth, stats> time_index;

  timeseries_base() {
    NODE_RESOLUTION_SKIP = bit_utils::highest_bit(branch_factor);
    LEAF_RESOLUTION = 64 - NODE_RESOLUTION_SKIP * depth;
    LEAF_TIME_RANGE = UINT64_C(1) << LEAF_RESOLUTION;
  }

  void init(size_t id) {
    std::string log_filename = "ts" + std::to_string(id);
    fprintf(stderr, "filename = %s\n", log_filename.c_str());
    log_.init(log_filename, "/home/ubuntu/dialog/data");
  }

  version_t append(const data_pt* pts, size_t len) {
    static auto update_stats =
        [](atomic::type<stats*>* s, version_t ver, const data_pt* pts, size_t len) {
          stats* old_s = atomic::load(s);
          atomic::store(s, (old_s == nullptr) ? new stats(ver, pts, len) : new stats(old_s, ver, pts, len));
        };

    version_t ver = log_.append(pts, len);
    uint64_t id1, id2;
    for (size_t i = 0; i < len;) {
      timestamp_t ts_block = _get_block(pts[i].timestamp);
      const data_pt* block_pts = pts + i;
      id1 = id2 = ver + i;
      while (++i < len && _get_block(pts[i].timestamp) == ts_block)
        id2++;
      ref_log* log = idx_(ts_block, update_stats, id2 + 1, block_pts,
                          id2 - id1 + 1);
      log->push_back_range(id1, id2);
      log_.flush(ver, len);
    }
    return ver;
  }

  version_t append(const data_pt* pts, size_t len, timestamp_t ts_block) {
    version_t ver = log_.append(pts, len);
    static auto update_stats =
        [](atomic::type<stats*>* s, version_t ver, const data_pt* pts, size_t len) {
          stats* old_s = atomic::load(s);
          atomic::store(s, (old_s == nullptr) ? new stats(ver, pts, len) : new stats(old_s, ver, pts, len));
        };

    ref_log* log = idx_(ts_block, update_stats, ver + len, pts, len);
    log->push_back_range(ver, ver + len - 1);
    log_.flush(ver, len);
    return ver;
  }

  void get_range(std::vector<data_pt>& out, timestamp_t ts1, timestamp_t ts2,
                 version_t version) {
    for (timestamp_t blk = _get_block(ts1); blk <= _get_block(ts2); blk++) {
      ref_log* ptrs = idx_.at(blk);
      size_t size = ptrs->size();
      for (size_t i = 0; i < size; i++) {
        data_ptr_t ptr = ptrs->at(i);
        if (ptr < version) {
          data_pt pt = log_.get(ptr);
          if (pt.timestamp >= ts1 && pt.timestamp <= ts2)
            out.push_back(pt);
        }
      }
    }
  }

  void get_statistical_range(std::vector<stats>& out, timestamp_t ts1,
                             timestamp_t ts2, timestamp_t resolution,
                             version_t version) {
    // For now, only support resolution upto leaf,
    // with 2^resolution aligned end points
    assert_throw(
        static_cast<size_t>(resolution) >= LEAF_RESOLUTION,
        "resolution: " << resolution << " LEAF_RESOLUTION: " << LEAF_RESOLUTION);
    assert_throw(ts1 % (INT64_C(1) << resolution) == 0,
                 "ts1 = " << ts1 << " resolution = " << resolution);
    assert_throw(ts2 % (INT64_C(1) << resolution) == 0,
                 "ts2 = " << ts2 << " resolution = " << resolution);

    size_t node_depth = depth
        - (resolution - LEAF_RESOLUTION) / NODE_RESOLUTION_SKIP;
    size_t node_resolution = 64 - NODE_RESOLUTION_SKIP * node_depth;
    size_t node_time_range = UINT64_C(1) << node_resolution;
    timestamp_t ts1_blk = ts1 / node_time_range;
    timestamp_t ts2_blk = ts2 / node_time_range;
    size_t agg_size = UINT64_C(1) << (resolution - node_resolution);

    stats agg_s;
    agg_s.version = version;
    for (timestamp_t blk = ts1_blk; blk < ts2_blk; blk++) {
      if (blk % agg_size == 0 && blk != ts1_blk) {
        out.push_back(agg_s);
        agg_s.reset();
      }

      stats* s = idx_.get_stats(blk, node_depth);
      if (s != nullptr) {
        // Get latest stats <= version
        while (s->version > version)
          s = atomic::load(&s->prv);
        agg_s.count += s->count;
        agg_s.sum += s->sum;
        agg_s.min = std::min(agg_s.min, s->min);
        agg_s.max = std::max(agg_s.max, s->max);
      }
    }
    out.push_back(agg_s);
  }

  data_pt get_nearest_value(timestamp_t ts, version_t version, bool direction) {
    timestamp_t ts_block = _get_block(ts);
    ref_log* ptrs = idx_.at(ts_block);
    if (direction)
      return _get_nearest_value_gt(ts, ptrs, version);
    return _get_nearest_value_lt(ts, ptrs, version);
  }

  void compute_diff(std::vector<data_pt>& pts, version_t from_version,
                    version_t to_version) {
    pts.reserve(to_version - from_version);
    for (version_t version = from_version; version < to_version; version++) {
      pts.push_back(log_.at(version));
    }
  }

 protected:
  static timestamp_t _get_block(timestamp_t ts) {
    return ts / LEAF_TIME_RANGE;
  }

  data_pt _get_nearest_value_lt(timestamp_t ts, ref_log* ptrs,
                                version_t version) {
    size_t size = ptrs->size();
    timestamp_t min = std::numeric_limits<timestamp_t>::max();
    data_pt min_pt;
    for (size_t i = 0; i < size; i++) {
      data_ptr_t ptr = ptrs->at(i);
      if (ptr < version) {
        data_pt pt = log_.get(ptr);
        if (pt.timestamp <= ts && (ts - pt.timestamp) < min) {
          min = (ts - pt.timestamp);
          min_pt = pt;
        }
      }
    }
    return min_pt;
  }

  data_pt _get_nearest_value_gt(timestamp_t ts, ref_log* ptrs,
                                version_t version) {
    size_t size = ptrs->size();
    timestamp_t min = std::numeric_limits<timestamp_t>::max();
    data_pt min_pt;
    for (size_t i = 0; i < size; i++) {
      data_ptr_t ptr = ptrs->at(i);
      if (ptr < version) {
        data_pt pt = log_.get(ptr);
        if (pt.timestamp >= ts && (pt.timestamp - ts) < min) {
          min = (ts - pt.timestamp);
          min_pt = pt;
        }
      }
    }
    return min_pt;
  }

  static size_t NODE_RESOLUTION_SKIP;
  static size_t LEAF_RESOLUTION;
  static size_t LEAF_TIME_RANGE;
  data_log log_;
  time_index idx_;
};

template<size_t branch_factor, size_t depth>
size_t timeseries_base<branch_factor, depth>::NODE_RESOLUTION_SKIP;

template<size_t branch_factor, size_t depth>
size_t timeseries_base<branch_factor, depth>::LEAF_RESOLUTION;

template<size_t branch_factor, size_t depth>
size_t timeseries_base<branch_factor, depth>::LEAF_TIME_RANGE;

template<size_t branch_factor = 256, size_t depth = 4>
class timeseries_t : public timeseries_base<branch_factor, depth> {
 public:
  timeseries_t()
      : timeseries_base<branch_factor, depth>(),
        read_tail_(0) {
  }

  version_t insert_values(const data_pt* pts, size_t len) {
    version_t ver = this->append(pts, len);
    update_read_tail(ver, len);
    return ver + len;
  }

  version_t insert_values(const data_pt* pts, size_t len,
                          timestamp_t ts_block) {
    version_t ver = this->append(pts, len, ts_block);
    update_read_tail(ver, len);
    return ver + len;
  }

  void get_range_latest(std::vector<data_pt>& out, timestamp_t ts1,
                        timestamp_t ts2) {
    this->get_range(out, ts1, ts2, num_entries());
  }

  void get_statistical_range_latest(std::vector<stats>& out, timestamp_t ts1,
                                    timestamp_t ts2, timestamp_t resolution) {
    this->get_statistical_range(out, ts1, ts2, resolution, num_entries());
  }

  data_pt get_nearest_value_latest(bool direction, timestamp_t ts) {
    return this->get_nearest_value(ts, num_entries(), direction);
  }

  uint64_t num_entries() const {
    return atomic::load(&read_tail_);
  }

 private:
  void update_read_tail(uint64_t ver, uint64_t cnt) {
    atomic::store(&read_tail_, ver + cnt);
  }

  atomic::type<uint64_t> read_tail_;
};

template<size_t branch_factor = 256, size_t depth = 4>
class timeseries_db {
 public:
  typedef timeseries_t<branch_factor, depth> ts;
  typedef timeseries_t<branch_factor, depth>* ts_ref;

  timeseries_db() = default;

  uuid_t add_stream() {
    ts* t = new ts();
    size_t pos = ts_.push_back(t);
    t->init(pos);
    return pos;
  }

  ts_ref& operator[](uuid_t uuid) {
    return ts_[uuid];
  }

 private:
  monolog::monolog_write_stalled<ts_ref> ts_;
};

}

#endif /* TIMESERIES_TIMESERIES_DB_H_ */

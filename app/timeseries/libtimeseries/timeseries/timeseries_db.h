#ifndef TIMESERIES_TIMESERIES_DB_H_
#define TIMESERIES_TIMESERIES_DB_H_

#include <thread>

#include "monolog.h"
#include "tiered_index.h"
#include "server/timeseries_db_types.h"

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

  stats()
      : version(UINT64_MAX),
        min(UINT64_MAX),
        max(UINT64_C(0)),
        sum(UINT64_C(0)),
        count(UINT64_C(0)) {

  }

  stats(version_t ver, value_t mn, value_t mx, value_t sm, value_t cnt)
      : version(ver),
        min(mn),
        max(mx),
        sum(sm),
        count(cnt) {
  }

  stats(version_t ver, const data_pt* pt, size_t len)
      : version(ver),
        count(len) {
    min = UINT64_MAX;
    max = UINT64_C(0);
    sum = 0;
    for (size_t i = 0; i < len; i++) {
      min = std::min(min, pt[i].value);
      max = std::max(max, pt[i].value);
      sum += pt[i].value;
    }
  }
};

template<size_t branch_factor = 1024, size_t depth = 4>
class timeseries_db_base {
 public:
  typedef monolog::monolog_relaxed_linear<data_pt> data_log;
  typedef monolog::monolog_relaxed_linear<data_ptr_t, 10, 1024> ptr_log;
  typedef datastore::index::tiered_index<ptr_log, branch_factor, depth> time_index;

  timeseries_db_base() = default;

  version_t append(const data_pt& pt) {
    version_t id = log_.push_back(pt);
    timestamp_t ts_block = pt.timestamp / BLOCK_TIME_RANGE;
    idx_[ts_block]->push_back(id);
    return id;
  }

  version_t append(const data_pt* pts, size_t len) {
    version_t id = log_.reserve(len);
    for (size_t i = 0; i < len; i++) {
      log_.set(id + i, pts[i]);
      timestamp_t ts_block = pts[i].timestamp / BLOCK_TIME_RANGE;
      idx_[ts_block]->push_back(id + i);
    }
    return id;
  }

  version_t append(const data_pt* pts, size_t len, timestamp_t ts_block) {
    version_t id = log_.reserve(len);
    for (size_t i = 0; i < len; i++)
      log_.set(id + i, pts[i]);
    idx_[ts_block]->push_back_range(id, id + len);
    return id;
  }

  uint64_t num_entries() const {
    return log_.size();
  }

 protected:
  template<typename validator>
  void _get_range(std::vector<data_pt>& results, timestamp_t start_ts,
                  timestamp_t end_ts, version_t version, validator&& validate) {
    timestamp_t start_ts_block = start_ts / BLOCK_TIME_RANGE;
    timestamp_t end_ts_block = end_ts / BLOCK_TIME_RANGE;
    for (timestamp_t ts_block = start_ts_block; ts_block <= end_ts_block;
        ts_block++) {
      ptr_log* ptrs = idx_.at(ts_block);
      size_t size = ptrs->size();
      for (size_t i = 0; i < size; i++) {
        data_ptr_t ptr = ptrs->at(i);
        if (ptr < version) {
          validate(ptr);
          data_pt pt = log_.get(ptr);
          if (pt.timestamp >= start_ts && pt.timestamp <= end_ts)
            results.push_back(pt);
        }
      }
    }
  }

  template<typename validator>
  data_pt _get_nearest_value(timestamp_t ts, version_t version, bool direction,
                             validator&& validate) {
    timestamp_t ts_block = ts / BLOCK_TIME_RANGE;
    ptr_log* ptrs = idx_.at(ts_block);
    if (direction)
      return _get_nearest_value_gt(ts, ptrs, version, validate);
    return _get_nearest_value_lt(ts, ptrs, version, validate);
  }

  template<typename validator>
  void _compute_diff(std::vector<data_pt>& pts, version_t from_version,
                     version_t to_version, validator&& validate) {
    pts.reserve(to_version - from_version);
    for (version_t version = from_version; version < to_version; version++) {
      validate(version);
      pts.push_back(log_.at(version));
    }
  }

  template<typename validator>
  data_pt _get_nearest_value_lt(timestamp_t ts, ptr_log* ptrs,
                                version_t version, validator&& validate) {
    size_t size = ptrs->size();
    timestamp_t min = std::numeric_limits<timestamp_t>::max();
    data_pt min_pt;
    for (size_t i = 0; i < size; i++) {
      data_ptr_t ptr = ptrs->at(i);
      if (ptr < version) {
        validate(ptr);
        data_pt pt = log_.get(ptr);
        if (pt.timestamp <= ts && (ts - pt.timestamp) < min) {
          min = (ts - pt.timestamp);
          min_pt = pt;
        }
      }
    }
    return min_pt;
  }

  template<typename validator>
  data_pt _get_nearest_value_gt(timestamp_t ts, ptr_log* ptrs,
                                version_t version, validator&& validate) {
    size_t size = ptrs->size();
    timestamp_t min = std::numeric_limits<timestamp_t>::max();
    data_pt min_pt;
    for (size_t i = 0; i < size; i++) {
      data_ptr_t ptr = ptrs->at(i);
      if (ptr < version) {
        validate(ptr);
        data_pt pt = log_.get(ptr);
        if (pt.timestamp >= ts && (pt.timestamp - ts) < min) {
          min = (ts - pt.timestamp);
          min_pt = pt;
        }
      }
    }
    return min_pt;
  }

  static size_t BLOCK_TIME_RANGE;
  data_log log_;
  time_index idx_;
};

template<size_t branch_factor, size_t depth>
size_t timeseries_db_base<branch_factor, depth>::BLOCK_TIME_RANGE = UINT64_C(1)
    << (64 - (utils::bit_utils::highest_bit(branch_factor) * depth));

template<size_t branch_factor = 1024, size_t depth = 4>
class timeseries_db_rs : public timeseries_db_base<branch_factor, depth> {
 public:
  timeseries_db_rs()
      : timeseries_db_base<branch_factor, depth>() {
  }

  version_t insert_values(const data_pt* pts, size_t len) {
    version_t ver = this->append(pts, len);
    valid_.set_bits(ver, len);
    return ver + len;
  }

  version_t insert_values(const data_pt* pts, size_t len,
                          timestamp_t ts_block) {
    version_t ver = this->append(pts, len, ts_block);
    valid_.set_bits(ver, len);
    return ver + len;
  }

  void get_range(std::vector<data_pt>& results, timestamp_t start_ts,
                 timestamp_t end_ts, version_t version) {
    this->_get_range(results, start_ts, end_ts, version,
                     [this](version_t v) {while (!valid_.get_bit(v));});
  }

  void get_range_latest(std::vector<data_pt>& results, timestamp_t start_ts,
                        timestamp_t end_ts) {
    this->_get_range(results, start_ts, end_ts, this->num_entries(),
                     [this](version_t v) {while (!valid_.get_bit(v));});
  }

  data_pt get_nearest_value(bool direction, timestamp_t ts, version_t version) {
    return this->_get_nearest_value(
        ts, version, direction,
        [this](version_t v) {while (!valid_.get_bit(v));});
  }

  data_pt get_nearest_value_latest(bool direction, timestamp_t ts) {
    return this->_get_nearest_value(
        ts, this->num_entries(), direction,
        [this](version_t v) {while (!valid_.get_bit(v));});
  }

  void compute_diff(std::vector<data_pt>& pts, version_t from_version,
                    version_t to_version) {
    this->_compute_diff(pts, from_version, to_version,
                        [this](version_t v) {while (!valid_.get_bit(v));});
  }

 private:
  monolog::monolog_bitvector valid_;
};

template<size_t branch_factor = 1024, size_t depth = 4>
class timeseries_db_ws : public timeseries_db_base<branch_factor, depth> {
 public:
  timeseries_db_ws()
      : timeseries_db_base<branch_factor, depth>(),
        validator_([](version_t v) {}),
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

  void get_range(std::vector<data_pt>& results, timestamp_t start_ts,
                 timestamp_t end_ts, version_t version) {
    this->_get_range(results, start_ts, end_ts, version, validator_);
  }

  void get_range_latest(std::vector<data_pt>& results, timestamp_t start_ts,
                        timestamp_t end_ts) {
    this->_get_range(results, start_ts, end_ts, this->num_entries(),
                     validator_);
  }

  data_pt get_nearest_value(bool direction, timestamp_t ts, version_t version) {
    return this->_get_nearest_value(ts, version, direction, validator_);
  }

  data_pt get_nearest_value_latest(bool direction, timestamp_t ts) {
    return this->_get_nearest_value(ts, this->num_entries(), direction,
                                    validator_);
  }

  void compute_diff(std::vector<data_pt>& pts, version_t from_version,
                    version_t to_version) {
    this->_compute_diff(pts, from_version, to_version, validator_);
  }

  uint64_t num_entries() const {
    return atomic::load(&read_tail_);
  }

 private:
  void update_read_tail(uint64_t tail, uint64_t cnt) {
    uint64_t old_tail = tail;
    while (!atomic::weak::cas(&read_tail_, &old_tail, tail + cnt)) {
      old_tail = tail;
      std::this_thread::yield();
    }
  }

  void (*validator_)(version_t version);
  atomic::type<uint64_t> read_tail_;
};

}

#endif /* TIMESERIES_TIMESERIES_DB_H_ */

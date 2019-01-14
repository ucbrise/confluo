#ifndef TEST_UNIVERSAL_SKETCH_TEST_H
#define TEST_UNIVERSAL_SKETCH_TEST_H

#include "container/data_log.h"
#include "container/sketch/universal_sketch.h"
#include "container/sketch/priority_queue.h"
#include "schema/schema.h"
#include "gtest/gtest.h"
#include "sketch_test_utils.h"

using namespace ::confluo::sketch;

class UniversalSketchTest : public testing::Test {
public:
  typedef std::unordered_map<int, uint64_t> hist_t;
  typedef pq<int, uint64_t> min_pq_t;

  static int64_t f0(int64_t x) {
    return 1;
  }

  static int64_t f1(int64_t x) {
    return x;
  }

  static int64_t f2(int64_t x) {
    return int64_t(x * x);
  }

  struct data_point {
    int64_t ts;
    int64_t val;

    data_point(uint64_t _ts, int64_t _val)
        : ts(_ts),
          val(_val) {
    }
  }__attribute__((packed));

  schema_t build_schema() {
    return schema_t(schema_builder().add_column(primitive_types::LONG_TYPE(), "long_field").get_columns());
  }

  min_pq_t get_heavy_hitters(hist_t hist, size_t k) {
    min_pq_t heavy_hitters;
    for (auto p : hist) {
      if (heavy_hitters.size() < k) {
        heavy_hitters.pushp(p.first, p.second);
      }
      else if (heavy_hitters.top().priority < p.second) {
        heavy_hitters.pop();
        heavy_hitters.pushp(p.first, p.second);
      }
    }
    return heavy_hitters;
  }

  void fill(data_log &log, const schema_t &schema, hist_t &hist, universal_sketch &sketch) {
    // Update sketch
    double avg_lat = 0;
    size_t i = 0;
    for (auto p : hist) {
      i++;
      auto pt = data_point(i, p.first);
      auto *data = reinterpret_cast<uint8_t *>(&pt);
      auto off = log.append(data, schema.record_size());
      auto r = schema.apply_unsafe(off, data);

      auto start = time_utils::cur_ns();
      if (p.second > 1)
        sketch.update(r, p.second - 1);
      auto stop = time_utils::cur_ns();
      avg_lat += (stop - start);

      if (log.size() % 2000 == 0) {
        LOG_INFO << log.size() << ", avg insertion lat = " << (avg_lat)/1000 << "ns";
        avg_lat = 0;
      }
    }
    // Last round of sketch updates
    for (auto p : hist) {
      i++;
      auto pt = data_point(i, p.first);
      auto *data = reinterpret_cast<uint8_t *>(&pt);
      auto off = log.append(data, schema.record_size());
      auto r = schema.apply_unsafe(off, data);
      sketch.update(r);
    }
  }

};

TEST_F(UniversalSketchTest, EstimateAccuracyTest) {

  hist_t hist;
  NormalGenerator(0, 100).sample(hist, 10000000);

  size_t l = 32;
  size_t b = 2700; // epsilon = 0.01
  size_t t = 32;
  size_t k = 10;

  schema_t schema = build_schema();
  data_log log("data_log", "/tmp", storage::IN_MEMORY);
  universal_sketch univ_sketch(l, b, t, k, &log, schema.columns()[1]);

}

TEST_F(UniversalSketchTest, GetHeavyHittersTest) {
  hist_t hist;
  NormalGenerator(0, 100).sample(hist, 10000000);

  // 0.01 margin of error
  size_t l = 32;
  size_t b = 2700; // epsilon = 0.01
  size_t t = 32;
  size_t k = 10;

  schema_t schema = build_schema();
  data_log log("data_log", "/tmp", storage::IN_MEMORY);
  universal_sketch univ_sketch(l, b, t, k, &log, schema.columns()[1]);
  fill(log, schema, hist, univ_sketch);
  auto actual_heavy_hitters = get_heavy_hitters(hist, k);

  auto hhs = univ_sketch.get_heavy_hitters(1);
  for (auto hh : hhs) {
    LOG_INFO << "[univ] " << hh.first << ": " << hh.second;
  }
  for (auto hh : actual_heavy_hitters) {
    auto key = std::to_string(hh.key);
    LOG_INFO << "[actual] " << hh.key << ": " << hh.priority << " vs [x] " << univ_sketch.estimate_frequency(key);
  }
//
//  for (auto p : hhs_actual) {
//    auto est = us.estimate_count(p.key);
//    auto error = std::abs(est - p.priority) * 1.0 / est;
//    LOG_INFO << "Error for " << p.key << ": " << error << " (" << est << " vs " << p.priority << ")";
//  }

}


#endif //TEST_UNIVERSAL_SKETCH_TEST_H

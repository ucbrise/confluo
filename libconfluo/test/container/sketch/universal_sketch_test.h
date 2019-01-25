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
  typedef thread_unsafe_pq<int, uint64_t> min_pq_t;

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
    // Simulate n-1 updates
    double latency_sum = 0;
    size_t i = 0;
    for (auto p : hist) {
      auto pt = data_point(i, p.first);
      auto *data = reinterpret_cast<uint8_t *>(&pt);
      auto off = log.append(data, schema.record_size());
      auto r = schema.apply_unsafe(off, data);

      if (p.second > 1) {
        auto start = time_utils::cur_ns();
        sketch.update(r, p.second - 1);
        auto stop = time_utils::cur_ns();
        latency_sum += (stop - start);
        i++;
      }
    }
    LOG_INFO << "Universal Sketch update latency: " << (latency_sum) / i << "ns";
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
  double epsilon = 0.01;
  double gamma = 0.05;
  size_t k = 10;

  hist_t hist;
  NormalGenerator(0, 100).sample(hist, 1000000);

  schema_t schema = build_schema();
  data_log log("data_log", "/tmp", storage::IN_MEMORY);
  universal_sketch univ_sketch(epsilon, gamma, k, &log, schema.columns()[1]);

  fill(log, schema, hist, univ_sketch);
  auto actual_heavy_hitters = get_heavy_hitters(hist, k);
  auto estimated_heavy_hitters = univ_sketch.get_heavy_hitters();

  // TODO introduce more reliable, deterministic checks
  for (const auto &hh : actual_heavy_hitters) {
    auto est = univ_sketch.estimate_frequency(std::to_string(hh.key));
    // (1 - epsilon) * actual <= est <= (1 + epsilon) * actual
    ASSERT_LE(size_t((1 - epsilon) * hh.priority), est);
    ASSERT_GE(size_t((1 + epsilon) * hh.priority), est);
  }
  for (const auto &hh : estimated_heavy_hitters) {
    // (1 - epsilon) * actual <= est <= (1 + epsilon) * actual
    auto actual = hist[std::stoi(hh.first)];
    ASSERT_GE(hh.second, size_t((1 - epsilon) * actual));
    ASSERT_LE(hh.second, size_t((1 + epsilon) * actual));
  }
}

TEST_F(UniversalSketchTest, GetHeavyHittersZipfTest) {
  hist_t hist;
  ZipfGenerator().sample(hist, 10000000);

  double epsilon = 0.01;
  double gamma = 0.05;
  size_t k = 10;
  // Conservatively check if we identified at least 40% of the heavy hitters correctly
  double target_identified = 0.4;

  schema_t schema = build_schema();
  data_log log("data_log", "/tmp", storage::IN_MEMORY);
  universal_sketch univ_sketch(epsilon, gamma, k, &log, schema.columns()[1]);
  fill(log, schema, hist, univ_sketch);
  auto actual_heavy_hitters = get_heavy_hitters(hist, k);

  size_t num_heavy_hitters_identified = 0;
  auto estimated_heavy_hitters = univ_sketch.get_heavy_hitters();
  for (const auto &hh : estimated_heavy_hitters) {
    if (actual_heavy_hitters.contains(std::stoi(hh.first))) {
      num_heavy_hitters_identified++;
    }
  }

  // TODO introduce more reliable, deterministic checks
  ASSERT_GE(num_heavy_hitters_identified * 1.0 / k, target_identified);
}

#ifdef STRESS_TEST
TEST_F(UniversalSketchTest, GetHeavyHittersGaussianTest) {
  hist_t hist;
  NormalGenerator(0, 100).sample(hist, 10000000);

  double epsilon = 0.01;
  double gamma = 0.05;
  size_t k = 10;
  // Conservatively check if we identified at least 30% of the heavy hitters correctly
  double target_identified = 0.3;

  schema_t schema = build_schema();
  data_log log("data_log", "/tmp", storage::IN_MEMORY);
  universal_sketch univ_sketch(epsilon, gamma, k, &log, schema.columns()[1]);
  fill(log, schema, hist, univ_sketch);
  auto actual_heavy_hitters = get_heavy_hitters(hist, k);

  size_t num_heavy_hitters_identified = 0;
  auto estimated_heavy_hitters = univ_sketch.get_heavy_hitters();
  for (const auto &hh : estimated_heavy_hitters) {
    if (actual_heavy_hitters.contains(std::stoi(hh.first))) {
      num_heavy_hitters_identified++;
    }
  }

  // TODO introduce more reliable, deterministic checks
  LOG_INFO << num_heavy_hitters_identified << " heavy hitters identified.";
  ASSERT_GE(num_heavy_hitters_identified * 1.0 / k, target_identified);
}
#endif

#endif //TEST_UNIVERSAL_SKETCH_TEST_H

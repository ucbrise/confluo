#ifndef TEST_UNIVERSAL_SKETCH_TEST_H
#define TEST_UNIVERSAL_SKETCH_TEST_H

#include "container/sketch/universal_sketch.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class UniversalSketchTest : public testing::Test {
public:
  static const int N = 10000;
  static const int mean = 0;
  static const int variance = 100;

  static int64_t f0(int64_t x) {
    return 1;
  }

  static int64_t f1(int64_t x) {
    return x;
  }

  static int64_t f2(int64_t x) {
    return x * x;
  }

  static void benchmark(size_t l, size_t t, size_t b, size_t k, double a, std::map<int, long int> hist) {

    universal_sketch<int> us(l, t, b, k, a);
    std::function<int64_t(int64_t)> g = f1;

    int64_t actual = 0;

    int64_t update_a = time_utils::cur_ns();
    for (auto p : hist) {
      for (int i = 0; i < p.second; i++) {
        us.update(p.first);
      }
      actual += g(p.second);
    }
    int64_t update_b = time_utils::cur_ns();

    int64_t eval_a = time_utils::cur_ns();
    int64_t g_sum = us.evaluate(g);
    int64_t eval_b = time_utils::cur_ns();

    for (auto p : hist) {
      ASSERT_EQ(us.estimate_count(p.first), p.second);
    }

    LOG_INFO << g_sum << " vs " << actual;
    LOG_INFO << "Storage size: " << us.storage_size()/1024 << " KB";
    LOG_INFO << "Error: " << double(std::abs(g_sum - actual))/actual;
    LOG_INFO << "Latency per update: " << (update_b - update_a)/1000 << " us";
    LOG_INFO << "Evaluation latency: " << (eval_b - eval_a)/1000 << " us";
    LOG_INFO << "";

  }
};

TEST_F(UniversalSketchTest, EstimateAccuracyTest) {

  std::ofstream out("univ_sketch_error.out");

  std::random_device rd;
  std::mt19937 e2(rd());
  std::normal_distribution<double> dist(mean, variance);
  std::map<int, long int> hist;

  for (int n = 0; n < N; n++) {
    hist[std::abs(std::round(dist(e2)))]++;
  }

  size_t l = 5;
  size_t t = 32;
  size_t b = 27000;
  size_t k = 100;
  double a = 0;

  for (size_t layers = 1; layers < l; layers++)
    benchmark(layers, t, b, k, a, hist);

}

const int UniversalSketchTest::N;

#endif //TEST_UNIVERSAL_SKETCH_TEST_H

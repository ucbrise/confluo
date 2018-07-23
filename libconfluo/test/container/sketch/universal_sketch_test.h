#ifndef TEST_UNIVERSAL_SKETCH_TEST_H
#define TEST_UNIVERSAL_SKETCH_TEST_H

#include "container/sketch/universal_sketch.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class UniversalSketchTest : public testing::Test {
public:
  static constexpr int N = 1000000;
  static constexpr int MEAN = 0;
  static constexpr int SD = 400;

  static constexpr int X0 = 1;
  static constexpr int X1 = 10000000;

  UniversalSketchTest()
          : dist(MEAN, SD),
            udist(0.0, 1.0) {
    std::random_device rd;
    e2 = std::mt19937(rd());
  }

  int sample_zipf(double alpha = 1.5) {
    static bool first = true;      // Static first time flag
    static double c = 0;          // Normalization constant
    static double* sum_probs;     // Pre-calculated sum of probabilities
    double z = 0.0;               // Uniform random number (0 < z < 1)
    int zipf_value = 0;           // Computed exponential value to be returned
    int i;                        // Loop counter
    int low, high, mid;           // Binary-search bounds

    // Compute normalization constant on first call only
    if (first) {
      for (i = X0; i <= X1; i++)
        c = c + (1.0 / pow((double) i, alpha));
      c = 1.0 / c;
      sum_probs = new double[X1+1];
      sum_probs[0] = 0;
      for (i = X0; i <= X1; i++)
        sum_probs[i] = sum_probs[i-1] + c / pow((double) i, alpha);
      first = false;
    }

    while ((z == 0) || (z == 1)) // (0 < z < 1)
      z = udist(e2);

    low = X0, high = X1, mid = 0;
    while (low <= high) {
      mid = std::floor((low+high)/2);
      if (sum_probs[mid] >= z && sum_probs[mid-1] < z) {
        zipf_value = mid;
        break;
      }
      else if (sum_probs[mid] >= z) {
        high = mid-1;
      }
      else {
        low = mid+1;
      }
    }
    assert((zipf_value >= X0) && (zipf_value <= X1));
    return(zipf_value);
  }

  double generate_zipf(std::map<int, long int>& hist) {
    for (int n = 0; n < N; n++)
      hist[std::abs(sample_zipf(1.2))]++;

    double l2_sq = 0.0;
    for (auto p : hist)
      l2_sq += (p.second * p.second);

    return std::sqrt(l2_sq);
  }

  static int64_t f0(int64_t x) {
    return 1;
  }

  static int64_t f1(int64_t x) {
    return x;
  }

  static int64_t f2(int64_t x) {
    return x * x;
  }

  static void eval(universal_sketch<int>& us, std::map<int, long int>& hist, size_t layers) {

    std::function<int64_t(int64_t)> g = f2;

    int64_t actual = 0;

    int64_t update_a = time_utils::cur_ns();
    for (auto p : hist) {
      for (int i = 0; i < p.second; i++) {
        us.update(p.first);
      }
      actual += g(p.second);
    }
    int64_t update_b = time_utils::cur_ns();

    for (size_t l = 1; l < layers; l++) {
      int64_t eval_a = time_utils::cur_ns();
      int64_t g_sum = us.evaluate(g, l);
      int64_t eval_b = time_utils::cur_ns();

      LOG_INFO << "Layers: " << l;
      LOG_INFO << g_sum << " vs " << actual;
      LOG_INFO << "Storage size: " << us.storage_size()/1024 << " KB";
      LOG_INFO << "Error: " << double(std::abs(g_sum - actual))/actual;
      //LOG_INFO << "Latency per update: " << (update_b - update_a)/1000 << " us";
      LOG_INFO << "Evaluation latency: " << (eval_b - eval_a)/1000 << " us";
      LOG_INFO << "";
    }

  }

 private:
  std::normal_distribution<double> dist;
  std::uniform_real_distribution<> udist;
  std::mt19937 e2;

};

TEST_F(UniversalSketchTest, EstimateAccuracyTest) {

  std::ofstream out("univ_sketch_error.out");

  std::random_device rd;
  std::mt19937 e2(rd());
  std::map<int, long int> hist;
  double l2 = generate_zipf(hist);

  size_t l = 12;
  size_t t = 64;
  size_t b = 27000;
  size_t k = 20;
  double a = 0;
  universal_sketch<int> us(l, t, b, k, a);
  eval(us, hist, l);

}

const int UniversalSketchTest::N;

#endif //TEST_UNIVERSAL_SKETCH_TEST_H

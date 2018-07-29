#ifndef TEST_COUNT_SKETCH_TEST_H_
#define TEST_COUNT_SKETCH_TEST_H_

#include <functional>

#include "container/sketch/count_sketch.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class CountSketchTest : public testing::Test {
 public:
  static constexpr int N = 1e6;

  static constexpr double MEAN = 0.0;
  static constexpr double SD = 500.0;

  static constexpr int X0 = 1;
  static constexpr int X1 = 1e7;

  CountSketchTest()
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

  double generate_zipf(std::map<int, uint64_t>& hist) {
    for (int n = 0; n < N; n++)
      hist[std::abs(sample_zipf(1.2))]++;

    double l2_sq = 0.0;
    for (auto p : hist)
      l2_sq += (p.second * p.second);

    return std::sqrt(l2_sq);
  }

  static void run(std::map<int, uint64_t>& hist, double epsilon) {
    double gamma = 0.001;
    size_t k = 100;

    heavy_hitter_set<int, int64_t> hhs;
    heavy_hitter_set<int, int64_t> hhs_actual;
    auto cs = count_sketch<int>::create_parameterized(epsilon, gamma);

    std::cout << "Updating...\n";

    // Forward to last updates
    for (auto p : hist) {
      if (p.second > 1)
        cs.update(p.first, p.second);
    }

    for (auto p : hist) {
      cs.update(p.first);
      int64_t est = cs.estimate(p.first);
      if (hhs.size() < k) {
        hhs.pushp(p.first, est);
      }
      else {
        int head = hhs.top().key_;
        int64_t head_priority = hhs.top().priority_;
        if (head_priority < est) {
          hhs.pop();
          hhs.pushp(p.first, est);
          assert_throw(hhs.top().priority_ >= head_priority,
                       std::to_string(hhs.top().priority_) + " > " + std::to_string(head_priority));
        }
      }
      if (hhs_actual.size() < k) {
        hhs_actual.pushp(p.first, est);
      } else {
        if (hhs_actual.top().priority_ < est) {
          hhs_actual.pop();
          hhs_actual.pushp(p.first, p.second);
        }
      }
    }

    int64_t smallest_actual = hhs_actual.top().priority_;
    for (auto hh : hhs) {
      ASSERT_LT((1 - epsilon) * smallest_actual, hist[hh.key_]);
    }
  }

 private:
  std::normal_distribution<double> dist;
  std::uniform_real_distribution<> udist;
  std::mt19937 e2;

};

const int CountSketchTest::N;

//TEST_F(CountSketchTest, EstimateAccuracyTest) {

//  double alpha = 0.01;
//  double epsilon[] = { 0.01, 0.02, 0.04, 0.08, 0.12 };

//  std::ofstream summary_out("sketch_test_summary.out");
//  std::ofstream out("sketch_error.out");

//  std::map<int, uint64_t> hist;
//  double l2 = generate_zipf(hist);

//  for (double e: epsilon)
//    run(hist, e);

//}

#endif /* TEST_COUNT_SKETCH_TEST */

#ifndef TEST_COUNT_SKETCH_TEST_H_
#define TEST_COUNT_SKETCH_TEST_H_

#include <functional>

#include "container/sketch/count_sketch.h"
#include "container/sketch/count_sketch_simple.h"
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

  template<typename T, typename P>
  static void bounded_pq_insert(pq<T, P> &queue, T key, P priority, size_t k) {
    assert_throw(queue.size() <= k, "Queue can't be larger than " + std::to_string(k) + " elements");
    assert_throw(!queue.contains(key), "Key " + std::to_string(key) + " is not unique");
    if (queue.size() < k) {
      queue.pushp(key, priority);
    }
    else if (queue.top().priority < priority) {
      queue.pop();
      queue.pushp(key, priority);
    }
  }

  static void run(std::map<int, uint64_t> &hist, double epsilon, double gamma, size_t k) {

    auto cs = count_sketch<int>::create_parameterized(epsilon, gamma);
    ASSERT_GT(cs.width(), 8 * k);
    for (auto p : hist) {
      cs.update(p.first, p.second);
    }

    pq<int, int64_t> hhs, hhs_actual;
    for (auto p : hist) {
      int64_t est = cs.estimate(p.first);
      bounded_pq_insert<int, int64_t>(hhs, p.first, est, k);
      bounded_pq_insert<int, int64_t>(hhs_actual, p.first, p.second, k);
    }

    int64_t smallest_actual = hhs_actual.top().priority;
    std::vector<double> errors;

    // Invariant defined by Charikhar count-sketch paper:
    // k elements such that every element i has actual frequency ni > (1 - e)nk
    for (auto hh : hhs) {
      ASSERT_GT(hist[hh.key], (1 - epsilon) * smallest_actual);
      auto error = std::abs(int64_t(hist[hh.key]) - hh.priority) * 1.0 / hist[hh.key];
      errors.push_back(error);
    }
  }

 private:
  std::normal_distribution<double> dist;
  std::uniform_real_distribution<> udist;
  std::mt19937 e2;

};

const int CountSketchTest::N;

/**
 * Tests that the CountSketch solves FindApproxTop(S, k, e) for b > 8k
 * http://www.cs.princeton.edu/courses/archive/spring04/cos598B/bib/CharikarCF.pdf
 */
TEST_F(CountSketchTest, InvariantTest) {
  // TODO run multiple trials and determine gamma empirically to avoid randomness in testing
  // for now use very small gamma to enforce invariant and remove randomness
  double g = 0.01;
  size_t k = 100;
  double epsilon[] = { 0.01, 0.02, 0.04 };

  std::map<int, uint64_t> hist;
  double l2 = generate_zipf(hist);

  for (double e : epsilon)
    run(hist, e, g, k);

}

#endif /* TEST_COUNT_SKETCH_TEST */

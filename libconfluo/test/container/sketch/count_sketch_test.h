#ifndef TEST_COUNT_SKETCH_TEST_H_
#define TEST_COUNT_SKETCH_TEST_H_

#include <functional>
#include <unordered_map>

#include "container/sketch/count_sketch.h"
#include "sketch_test_utils.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class CountSketchTest : public testing::Test {
 public:
  typedef std::unordered_map<int, uint64_t> histogram_t;

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

  static void run(histogram_t &hist, double epsilon, double gamma, size_t k) {
    auto cs = count_sketch<int>::create_parameterized(epsilon, gamma);
    ASSERT_GT(cs.width(), 8 * k);

    auto start = utils::time_utils::cur_ns();
    for (auto p : hist) {
      cs.update(p.first, p.second);
    }
    auto stop = utils::time_utils::cur_ns();
    LOG_INFO << "Count Sketch update latency: " << (stop - start) / hist.size();

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

};

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

  histogram_t hist;
  ZipfGenerator().sample(hist, 1000000);

  for (double e : epsilon)
    run(hist, e, g, k);

}

#endif /* TEST_COUNT_SKETCH_TEST */

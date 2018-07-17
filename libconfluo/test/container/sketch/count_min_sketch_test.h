#ifndef TEST_CONTAINER_SKETCH_COUNT_MIN_SKETCH_TEST_H_
#define TEST_CONTAINER_SKETCH_COUNT_MIN_SKETCH_TEST_H_

#include "container/sketch/count_min_sketch.h"
#include "container/sketch/universal_monitor.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class CountSketchTest : public testing::Test {
 public:
  static const int N = 1000;
};

const int CountSketchTest::N;

//TEST_F(CountSketchTest, SimpleHeavyFlowsEstimateTest) {
//
//  hash_manager manager;
//  size_t A[10][2] = {
//      {1, 3543},
//      {2, 7932},
//      {3, 8234},
//      {4, 48},
//      {5, 58},
//      {6, 238},
//      {7, 732},
//      {8, 10038},
//      {9, 78},
//      {327, 78923}
//  };
//
//  auto cs1 = count_sketch<int>::create_parameterized(0.1, 0.1, manager);
//  for (int i = 0; i < 10; i++) {
//    for (int j = 0; j < A[i][1]; j++) {
//      cs1.update(A[i][0]);
//    }
//  }
//  for (int i = 0; i < 10; i++) {
//    ASSERT_EQ(cs1.estimate(A[i][0]), A[i][1]);
//  }
//}
//

TEST_F(CountSketchTest, EstimateTest) {
  hash_manager manager;
  auto cs = count_sketch<int>::create_parameterized(0.01, 0.01, manager);

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < i; j++)
      cs.update(i);
  }

  for (int i = N - 1; i >= 0; i--) {
    ASSERT_EQ(cs.estimate(i), i);
  }
}

//TEST_F(CountSketchTest, BenchTest) {
//  hash_manager monitor_manager;
//  hash_manager sketch_manager;
//  double hh_thresh = 0.1;
//  size_t num_hh = 10;
//  universal_monitor<size_t> univ_mon = universal_monitor<size_t>::create_parameterized(0.01, 0.01, hh_thresh, num_hh,
//                                                                                   monitor_manager,
//                                                                                   sketch_manager);
//  size_t num_ops = 0;
//  size_t time = 0;
//  for (size_t i = 0; i < N; i++) {
//    for (int j = 0; j < i; j++) {
//      int64_t start = utils::time_utils::cur_ns();
//      univ_mon.update(i);
//      int64_t stop = utils::time_utils::cur_ns();
//      time += (stop - start);
//      num_ops++;
//    }
//  }
//
//  LOG_INFO << "Avg Latency: " << time/num_ops << "ns.";
//
//}

//TEST_F(CountSketchTest, UniversalMonitorTest) {
//  hash_manager monitor_manager;
//  hash_manager sketch_manager;
//  double hh_thresh = 0.1;
//  size_t num_hh = 10;
//  universal_monitor<int> univ_mon = universal_monitor<int>::create_parameterized(0.01, 0.01, hh_thresh, num_hh,
//                                                                                 monitor_manager,
//                                                                                 sketch_manager);
//
//  size_t total = 0;
//  int A[11][2] = {
//      {0, 0},
//      {1, 3543},
//      {2, 7932},
//      {3, 32234},
//      {4, 48},
//      {5, 58},
//      {6, 238},
//      {7, 732},
//      {8, 10038},
//      {9, 78},
//      {327, 78923}
//  };
//  size_t cdf[11] = {0,0,0,0,0,0,0,0,0,0,0};
//
//  for (size_t i = 1; i < 11; i++) {
//    total += A[i][1];
//    cdf[i] = total;
//  }
//
//  for (int i = 0; i < 10000; i++) {
//      size_t rand = utils::rand_utils::rand_int64(total);
//      for (int k = 1; k < 11; k++) {
//        if (rand > cdf[k - 1] && rand < cdf[k]) {
//          univ_mon.update(A[k][0]);
//        }
//    }
//  }
//
//  std::vector<atomic::type<int>>& hhs = univ_mon.get_heavy_hitters();
//  for (int i = 0; i < hhs.size(); i++) {
//    LOG_INFO << atomic::load(&hhs[i]);
//  }
//
//}

#endif /* TEST_CONTAINER_SKETCH_COUNT_MIN_SKETCH_TEST_H_ */

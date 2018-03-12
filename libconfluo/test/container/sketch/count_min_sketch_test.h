#ifndef TEST_CONTAINER_SKETCH_COUNT_MIN_SKETCH_TEST_H_
#define TEST_CONTAINER_SKETCH_COUNT_MIN_SKETCH_TEST_H_

#include "container/sketch/count_min_sketch.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class CountMinSketchTest : public testing::Test {
 public:
  static const int N = 100;
};

const int CountMinSketchTest::N;

TEST_F(CountMinSketchTest, EstimateTest) {
  hash_manager manager;
  auto cms = count_min_sketch<int>::create_parameterized(0.01, 0.01, manager);
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < i; j++)
      cms.update(i);
  }

  for (int i = 0; i < N; i++) {
    ASSERT_EQ(cms.estimate(i), i); // TODO account for errors
  }
}

#endif /* TEST_CONTAINER_SKETCH_COUNT_MIN_SKETCH_TEST_H_ */

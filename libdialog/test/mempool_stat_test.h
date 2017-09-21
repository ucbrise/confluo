#ifndef TEST_MEMPOOL_STAT_TEST_H_
#define TEST_MEMPOOL_STAT_TEST_H_

#include "mempool_stat.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class MempoolStatTest : public testing::Test {
};

TEST_F(MempoolStatTest, IncrementGetTest) {

  mempool_stat* stat = new mempool_stat();
  ASSERT_EQ(stat->get(), 0);

  stat->increment(8);
  stat->increment(16);

  ASSERT_EQ(stat->get(), 24);

}

TEST_F(MempoolStatTest, DecrementGetTest) {

  mempool_stat* stat = new mempool_stat();

  stat->increment(1024);
  stat->decrement(1000);

  ASSERT_EQ(stat->get(), 24);

}
#endif /* TEST_MEMPOOL_STAT_TEST_H_ */

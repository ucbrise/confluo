#ifndef TEST_MEMPOOL_STAT_TEST_H_
#define TEST_MEMPOOL_STAT_TEST_H_

#include "gtest/gtest.h"
#include "memory_stat.h"

using namespace ::confluo;

class MemoryStatTest : public testing::Test {
};

TEST_F(MemoryStatTest, IncrementGetTest) {

  memory_stat stat;
  ASSERT_EQ(stat.get(), 0);

  stat.increment(8);
  stat.increment(16);

  ASSERT_EQ(stat.get(), 24);

}

TEST_F(MemoryStatTest, DecrementGetTest) {

  memory_stat stat;

  stat.increment(1024);
  stat.decrement(1000);

  ASSERT_EQ(stat.get(), 24);

}

#endif /* TEST_MEMPOOL_STAT_TEST_H_ */

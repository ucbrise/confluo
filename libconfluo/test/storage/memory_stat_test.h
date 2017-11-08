#ifndef CONFLUO_TEST_MEMORY_STAT_TEST_H_
#define CONFLUO_TEST_MEMORY_STAT_TEST_H_

#include "storage/memory_stat.h"
#include "gtest/gtest.h"

using namespace ::confluo;

class MemoryStatTest : public testing::Test {
};

TEST_F(MemoryStatTest, IncrementGetTest) {

  storage::memory_stat stat;
  ASSERT_EQ(stat.get(), 0);

  stat.increment(8);
  stat.increment(16);

  ASSERT_EQ(stat.get(), 24);

}

TEST_F(MemoryStatTest, DecrementGetTest) {

  storage::memory_stat stat;

  stat.increment(1024);
  stat.decrement(1000);

  ASSERT_EQ(stat.get(), 24);

}

#endif /* CONFLUO_TEST_MEMORY_STAT_TEST_H_ */

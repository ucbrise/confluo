#ifndef CONFLUO_PRIORITY_QUEUE_TEST_H
#define CONFLUO_PRIORITY_QUEUE_TEST_H

#include "container/sketch/priority_queue.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class PriorityQueueTest : public testing::Test {

};

TEST_F(PriorityQueueTest, TestPushPopTop) {

  heavy_hitter_set<int, int> hhs;
  hhs.pushp(0, 5);
  hhs.pushp(1, 3);
  hhs.pushp(2, 9);
  hhs.pushp(3, 2);
  hhs.pushp(4, -1);

  ASSERT_EQ(hhs.size(), 5);
  ASSERT_EQ(hhs.top().key_, 4);
  ASSERT_EQ(hhs.top().priority_, -1);

  hhs.pop();

  ASSERT_EQ(hhs.size(), 4);
  ASSERT_EQ(hhs.top().key_, 3);
  ASSERT_EQ(hhs.top().priority_, 2);

  hhs.pop();
  hhs.pop();

  ASSERT_EQ(hhs.size(), 2);
  ASSERT_EQ(hhs.top().key_, 0);
  ASSERT_EQ(hhs.top().priority_, 5);

}

TEST_F(PriorityQueueTest, TestRemoveIfExists) {

  heavy_hitter_set<int, int> hhs;
  hhs.pushp(0, 5);
  hhs.pushp(1, 3);
  hhs.pushp(2, 9);
  hhs.pushp(3, 2);
  hhs.pushp(4, -1);

  hhs.remove_if_exists(3);
  hhs.pop();

  ASSERT_EQ(hhs.size(), 3);
  ASSERT_EQ(hhs.top().key_, 1);

}


#endif //CONFLUO_PRIORITY_QUEUE_TEST_H

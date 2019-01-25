#ifndef CONFLUO_PRIORITY_QUEUE_TEST_H
#define CONFLUO_PRIORITY_QUEUE_TEST_H

#include "container/sketch/priority_queue.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class PriorityQueueTest : public testing::Test {

};

TEST_F(PriorityQueueTest, TestPushPopTop) {

  thread_unsafe_pq<int, int> hhs;
  hhs.pushp(0, 5);
  hhs.pushp(1, 3);
  hhs.pushp(2, 9);
  hhs.pushp(3, 2);
  hhs.pushp(4, -1);

  ASSERT_EQ(hhs.size(), 5);
  ASSERT_EQ(hhs.top().key, 4);
  ASSERT_EQ(hhs.top().priority, -1);

  hhs.pop();

  ASSERT_EQ(hhs.size(), 4);
  ASSERT_EQ(hhs.top().key, 3);
  ASSERT_EQ(hhs.top().priority, 2);

  hhs.pop();
  hhs.pop();

  ASSERT_EQ(hhs.size(), 2);
  ASSERT_EQ(hhs.top().key, 0);
  ASSERT_EQ(hhs.top().priority, 5);

}

TEST_F(PriorityQueueTest, TestRemoveIfExists) {

  thread_unsafe_pq<int, int> hhs;
  hhs.pushp(0, 5);
  hhs.pushp(1, 3);
  hhs.pushp(2, 9);
  hhs.pushp(3, 2);
  hhs.pushp(4, -1);

  hhs.remove_if_exists(3);
  hhs.pop();

  ASSERT_EQ(hhs.size(), 3);
  ASSERT_EQ(hhs.top().key, 1);

}


TEST_F(PriorityQueueTest, TestContains) {

  thread_unsafe_pq<int, int> hhs;
  ASSERT_EQ(hhs.contains(3), false);

  hhs.pushp(0, 5);
  hhs.pushp(1, 3);
  hhs.pushp(2, 9);
  hhs.pushp(3, 2);
  hhs.pushp(4, -1);

  ASSERT_EQ(hhs.contains(3), true);
  ASSERT_EQ(hhs.contains(10), false);

}

#endif //CONFLUO_PRIORITY_QUEUE_TEST_H

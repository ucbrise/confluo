#ifndef CONFLUO_TEST_TASK_TEST_H_
#define CONFLUO_TEST_TASK_TEST_H_

#include "gtest/gtest.h"

#include "threads/task_pool.h"

using namespace confluo;

class TaskTest: public testing::Test {
};

TEST_F(TaskTest, AddTaskTest) {
  task_pool p;

  auto fut = p.submit([]() -> int {
    return 1;
  });

  int val = fut.get();

  ASSERT_EQ(1, val);
}

#endif /* CONFLUO_TEST_TASK_TEST_H_ */

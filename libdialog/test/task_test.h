#ifndef TEST_TASK_TEST_H_
#define TEST_TASK_TEST_H_

#include "task_worker.h"
#include "gtest/gtest.h"
#include "task_pool.h"

using namespace dialog;

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

#endif /* TEST_TASK_TEST_H_ */

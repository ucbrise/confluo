#ifndef DIALOG_PERIODIC_TASK_TEST_H_
#define DIALOG_PERIODIC_TASK_TEST_H_

#include "periodic_task.h"

#include "gtest/gtest.h"

class PeriodicTaskTest: public testing::Test {

};

TEST_F(PeriodicTaskTest, DummyTaskTest) {
  periodic_task task("dummy");
  std::vector<int> vec;
  int num = 0;
  task.start([&vec, &num] {
    vec.push_back(num);
    num++;
  }, 1);
  sleep(1);
  task.stop();
  for (int i = 0; i < static_cast<int>(vec.size()); i++) {
    ASSERT_EQ(vec[i], i);
  }
}

#endif /* DIALOG_PERIODIC_TASK_TEST_H_ */

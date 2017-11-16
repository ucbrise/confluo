#ifndef TEST_THREAD_MANAGER_TEST_H_
#define TEST_THREAD_MANAGER_TEST_H_

#include "thread_manager.h"
#include "gtest/gtest.h"

using namespace ::confluo;

class ThreadManagerTest : public testing::Test {

};

TEST_F(ThreadManagerTest, RegisterDeregisterTest) {
  int id = thread_manager::register_thread();
  ASSERT_TRUE(-1 != id);
  ASSERT_EQ(id, thread_manager::get_id());
  ASSERT_EQ(id, thread_manager::deregister_thread());
}

TEST_F(ThreadManagerTest, MultiThreadedRegisterDeregisterTest) {
  std::vector<std::thread> threads;
  for (int i = 0; i < thread_manager::get_max_concurrency(); i++) {
    threads.push_back(std::thread([] {
      int id = thread_manager::register_thread();
      ASSERT_TRUE(-1 != id);
      ASSERT_TRUE(id < constants::HARDWARE_CONCURRENCY);
      ASSERT_EQ(id, thread_manager::get_id());
      ASSERT_EQ(id, thread_manager::deregister_thread());
    }));
  }

  for (auto& t : threads) {
    if (t.joinable())
      t.join();
  }

}

#endif /* TEST_THREAD_MANAGER_TEST_H_ */

#include "log_store.h"
#include "gtest/gtest.h"
#include "coordinator.h"

using namespace ::datastore;
using namespace ::datastore::dependent;

#define DATA_SIZE 100

class CoordinatorTest : public testing::Test {
 public:
  static const size_t kNumStores = 3;
  static const size_t kNumWrites = 10000;

  CoordinatorTest() {
    for (size_t i = 0; i < DATA_SIZE; i++)
      data_[i] = i % 256;
  }

  template<typename data_store>
  void do_writes(std::vector<data_store>& stores) {
    std::vector<std::thread> workers;
    for (data_store& store : stores) {
      workers.push_back(std::thread([&store, this]() {
        for (size_t i = 0; i < kNumWrites; i++) {
          store.append(data_, DATA_SIZE);
        }
      }));
    }

    for (std::thread& worker : workers) {
      worker.join();
    }
  }

 protected:
  uint8_t data_[DATA_SIZE];
};

TEST_F(CoordinatorTest, SnapshotRS) {
  using data_store = datastore::log_store<in_memory, read_stalled>;
  for (uint64_t sleep_us = 0; sleep_us <= 100; sleep_us += 20) {
    std::vector<data_store> stores(kNumStores);
    datastore::coordinator<data_store> coord(stores, sleep_us);
    bool success = coord.start();
    ASSERT_TRUE(success);
    do_writes(stores);
    success = coord.stop();
    ASSERT_TRUE(success);
  }
}

TEST_F(CoordinatorTest, SnapshotWS) {
  using data_store = datastore::log_store<in_memory, write_stalled>;
  for (uint64_t sleep_us = 0; sleep_us <= 100; sleep_us += 20) {
    std::vector<data_store> stores(kNumStores);
    datastore::coordinator<data_store> coord(stores, sleep_us);
    bool success = coord.start();
    ASSERT_TRUE(success);
    do_writes(stores);
    success = coord.stop();
    ASSERT_TRUE(success);
  }
}

#ifndef TEST_MONOLOG_LINEAR_ARCHIVAL_TEST_H_
#define TEST_MONOLOG_LINEAR_ARCHIVAL_TEST_H_

#include "monolog_linear_archiver.h"
#include "container/monolog/monolog_linear.h"
#include "read_tail.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class DataArchivalTest : public testing::Test {

 public:
  static const size_t BLOCK_SIZE = 1024;
  static const size_t MAX_BLOCKS = 8;
  static const uint64_t ARRAY_SIZE = MAX_BLOCKS * BLOCK_SIZE;

  typedef monolog_linear<uint8_t, MAX_BLOCKS, BLOCK_SIZE, 1024> small_monolog_linear;
  typedef archival::monolog_linear_archiver<uint8_t, MAX_BLOCKS, BLOCK_SIZE, 1024> small_monolog_archiver;

  void write_to_log(small_monolog_linear& log) {
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
      log.set(i, i % 256);
    }
  }

  void verify(small_monolog_linear& log, size_t start, size_t stop) {
    for (size_t i = start; i < stop; i++) {
      ASSERT_EQ(log.get(i), i % 256);
    }
  }

};

TEST_F(DataArchivalTest, FlushTest) {
  read_tail rt("/tmp", storage::IN_MEMORY);
  small_monolog_linear log("log", "/tmp", storage::IN_MEMORY);
  small_monolog_archiver archiver("data_archives", "/tmp", rt, &log);

  write_to_log(log);
  rt.advance(0, ARRAY_SIZE * sizeof(uint8_t));

  archiver.archive(2 * BLOCK_SIZE);
  // TODO read from file

  archiver.archive(4 * BLOCK_SIZE);
  // TODO read from file

  archiver.archive(8 * BLOCK_SIZE);
  // TODO read from file
}

TEST_F(DataArchivalTest, PtrSwapTest) {
  read_tail rt("/tmp", storage::IN_MEMORY);
  small_monolog_linear log("log", "/tmp", storage::IN_MEMORY);
  small_monolog_archiver archiver("data_archives", "/tmp", rt, &log);

  write_to_log(log);
  rt.advance(0, ARRAY_SIZE * sizeof(uint8_t));

  archiver.archive(2 * BLOCK_SIZE);
  verify(log, 0, 2 * BLOCK_SIZE);

  archiver.archive(4 * BLOCK_SIZE);
  verify(log, 2 * BLOCK_SIZE, 4 * BLOCK_SIZE);

  archiver.archive(8 * BLOCK_SIZE);
  verify(log, 4 * BLOCK_SIZE, 8 * BLOCK_SIZE);
}

TEST_F(DataArchivalTest, ArchivePastReadTailTest) {
  read_tail rt("/tmp", storage::IN_MEMORY);
  small_monolog_linear log("log", "/tmp", storage::IN_MEMORY);
  small_monolog_archiver archiver("data_archives", "/tmp", rt, &log);

  write_to_log(log);
  rt.advance(0, 7 * BLOCK_SIZE);

  archiver.archive(8 * BLOCK_SIZE);
  verify(log, 0, 7 * BLOCK_SIZE);

  // make sure last block wasn't archived
  storage::read_only_ptr<uint8_t> ptr;
  log.ptr(8 * BLOCK_SIZE, ptr);
  ASSERT_EQ(storage::ptr_metadata::get(ptr.get())->state_, storage::state_type::D_IN_MEMORY);
}


#endif /* TEST_MONOLOG_LINEAR_ARCHIVAL_TEST_H_ */

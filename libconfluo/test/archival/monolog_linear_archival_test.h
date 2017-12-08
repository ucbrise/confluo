#ifndef TEST_MONOLOG_LINEAR_ARCHIVAL_TEST_H_
#define TEST_MONOLOG_LINEAR_ARCHIVAL_TEST_H_

#include "gtest/gtest.h"

#include "storage/encoder.h"
#include "container/monolog/monolog_linear.h"
#include "archival/monolog_linear_archiver.h"

using namespace ::confluo;

class DataArchivalTest : public testing::Test {

 public:
  static const size_t BLOCK_SIZE = 1024;
  static const size_t MAX_BLOCKS = 8;
  static const uint64_t ARRAY_SIZE = MAX_BLOCKS * BLOCK_SIZE;

  typedef storage::encoded_ptr<uint8_t> encoded_ptr_t;
  typedef monolog_linear<uint8_t, MAX_BLOCKS, BLOCK_SIZE, 1024> small_monolog_linear;
  typedef archival::monolog_linear_archiver<uint8_t, archival::encoding_type::IDENTITY, MAX_BLOCKS,
                                            BLOCK_SIZE, 1024> small_monolog_archiver;

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

/**
 * Verifies that data is written to disk correctly.
 */
TEST_F(DataArchivalTest, ArchivalTest) {
  small_monolog_linear log("log", "/tmp", storage::IN_MEMORY);
  small_monolog_archiver archiver("/tmp/data_log/", log);

  write_to_log(log);

  archiver.archive(2 * BLOCK_SIZE);
  // TODO read from file

  archiver.archive(4 * BLOCK_SIZE);
  // TODO read from file

  archiver.archive(8 * BLOCK_SIZE);
  // TODO read from file
}

/**
 * Verifies that monolog pointer is swapped.
 */
TEST_F(DataArchivalTest, PtrSwapTest) {
  small_monolog_linear log("log", "/tmp", storage::IN_MEMORY);
  small_monolog_archiver archiver("/tmp/data_log/", log);

  write_to_log(log);

  archiver.archive(2 * BLOCK_SIZE);
  verify(log, 0, 2 * BLOCK_SIZE);

  archiver.archive(4 * BLOCK_SIZE);
  verify(log, 2 * BLOCK_SIZE, 4 * BLOCK_SIZE);

  archiver.archive(8 * BLOCK_SIZE);
  verify(log, 4 * BLOCK_SIZE, 8 * BLOCK_SIZE);
}

#endif /* TEST_MONOLOG_LINEAR_ARCHIVAL_TEST_H_ */

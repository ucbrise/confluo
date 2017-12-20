#ifndef TEST_MONOLOG_LINEAR_ARCHIVAL_TEST_H_
#define TEST_MONOLOG_LINEAR_ARCHIVAL_TEST_H_

#include "gtest/gtest.h"

#include "storage/encoder.h"
#include "container/monolog/monolog_linear.h"
#include "archival/monolog_linear_archiver.h"

using namespace ::confluo;

class MonologLinearArchivalTest : public testing::Test {

 public:
  static const size_t BUCKET_SIZE = 1024;
  static const size_t MAX_BUCKETS = 8;
  static const uint64_t ARRAY_SIZE = MAX_BUCKETS * BUCKET_SIZE;

  typedef storage::encoded_ptr<uint8_t> encoded_ptr_t;
  typedef storage::read_only_encoded_ptr<uint8_t> read_only_ptr_t;
  typedef monolog_linear<uint8_t, MAX_BUCKETS, BUCKET_SIZE, 1024> small_monolog_linear;
  typedef archival::monolog_linear_archiver<uint8_t, archival::encoding_type::IDENTITY, MAX_BUCKETS,
                                            BUCKET_SIZE, 1024> small_monolog_archiver;

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

  void verify_swap(small_monolog_linear& log, size_t start, size_t stop) {
    read_only_ptr_t bucket_ptr;
    for (size_t i = 0; i < stop; i += BUCKET_SIZE) {
      log.ptr(i, bucket_ptr);
      ASSERT_EQ(storage::ptr_metadata::get(bucket_ptr.get().ptr())->state_, storage::state_type::D_ARCHIVED);
    }
  }

};

/**
 * Verifies that data is written to disk correctly.
 */
TEST_F(MonologLinearArchivalTest, ArchivalTest) {
  small_monolog_linear log("log", "/tmp", storage::IN_MEMORY);
  small_monolog_archiver archiver("/tmp/data_log/", &log);

  write_to_log(log);

  archiver.archive(2 * BUCKET_SIZE);
  // TODO read from file

  archiver.archive(4 * BUCKET_SIZE);
  // TODO read from file

  archiver.archive(8 * BUCKET_SIZE);
  // TODO read from file
}

/**
 * Verifies that monolog pointer is swapped.
 */
TEST_F(MonologLinearArchivalTest, PtrSwapTest) {
  small_monolog_linear log("log", "/tmp", storage::IN_MEMORY);
  small_monolog_archiver archiver("/tmp/data_log/", &log);

  write_to_log(log);

  archiver.archive(2 * BUCKET_SIZE);
  verify(log, 0, 2 * BUCKET_SIZE);
  verify_swap(log, 0, 2 * BUCKET_SIZE);

  archiver.archive(4 * BUCKET_SIZE);
  verify(log, 2 * BUCKET_SIZE, 4 * BUCKET_SIZE);
  verify_swap(log, 2 * BUCKET_SIZE, 4 * BUCKET_SIZE);

  archiver.archive(8 * BUCKET_SIZE);
  verify(log, 4 * BUCKET_SIZE, 8 * BUCKET_SIZE);
  verify_swap(log, 4 * BUCKET_SIZE, 8 * BUCKET_SIZE);
}

#endif /* TEST_MONOLOG_LINEAR_ARCHIVAL_TEST_H_ */

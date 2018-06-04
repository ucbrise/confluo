#ifndef TEST_ARCHIVAL_LOAD_MONOLOG_LINEAR_TEST_H_
#define TEST_ARCHIVAL_LOAD_MONOLOG_LINEAR_TEST_H_

#include "gtest/gtest.h"

#include "archival/load_utils.h"
#include "container/monolog/monolog_linear.h"
#include "archival/monolog_linear_archiver.h"

using namespace ::confluo;
using namespace ::confluo::archival;

class MonologLinearLoadTest : public testing::Test {

 public:
  static const size_t BUCKET_SIZE = 1024;
  static const size_t MAX_BUCKETS = 8;
  static const size_t BUF_SIZE = 1024;
  static const uint64_t ARRAY_SIZE = MAX_BUCKETS * BUCKET_SIZE;

  typedef storage::encoded_ptr<uint8_t> encoded_ptr_t;
  typedef storage::read_only_encoded_ptr<uint8_t> read_only_ptr_t;
  typedef monolog_linear<uint8_t, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE> small_monolog_linear;
  typedef monolog_linear_archiver<uint8_t, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE> small_monolog_archiver;

  void write_to_log(small_monolog_linear &log) {
    uint8_t buf[ARRAY_SIZE];
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
      buf[i] = i % 256;
    }
    log.append(buf, ARRAY_SIZE);
  }

  void verify(small_monolog_linear &log, size_t start, size_t stop) {
    for (size_t i = start; i < stop; i++) {
      ASSERT_EQ(log.get(i), i % 256);
    }
  }

};

TEST_F(MonologLinearLoadTest, FullyArchivedLogLoadTest) {

  std::string path = "/tmp/data_log/";
  file_utils::clear_dir(path);

  {
    small_monolog_linear log("log", "/tmp", storage::IN_MEMORY);
    small_monolog_archiver archiver(path, &log);
    write_to_log(log);
    archiver.archive(2 * BUCKET_SIZE);
    archiver.archive(4 * BUCKET_SIZE);
    archiver.archive(6 * BUCKET_SIZE);
    archiver.archive(8 * BUCKET_SIZE);
  }

  small_monolog_linear recovered_log("log", "/tmp", storage::IN_MEMORY);
  monolog_linear_load_utils::load<uint8_t, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE>(path, recovered_log);
  verify(recovered_log, 0, ARRAY_SIZE);
}

#endif /* TEST_ARCHIVAL_LOAD_MONOLOG_LINEAR_TEST_H_ */

#ifndef TEST_INDEX_ARCHIVAL_TEST_H_
#define TEST_INDEX_ARCHIVAL_TEST_H_

#include <thread>

#include "archival/index_log_archiver.h"
#include "index_log.h"
#include "container/radix_tree.h"
#include "storage/ptr_aux_block.h"
#include "schema/schema.h"
#include "gtest/gtest.h"

using namespace ::confluo;

class IndexArchivalTest : public testing::Test {
 public:

  static const size_t NUM_BUCKETS_PER_REFLOG = 8;
  static const size_t REFLOG_SIZE = NUM_BUCKETS_PER_REFLOG * reflog_constants::BUCKET_SIZE;

  static void fill(index::radix_index& index) {
    uint64_t accum = 0;
    for (int32_t i = 0; i < 128; i++) {
      byte_string key = byte_string(i * 8);
      for (uint64_t j = accum; j < accum + REFLOG_SIZE; j++) {
        index.insert(key, j);
      }
      ASSERT_EQ(REFLOG_SIZE, index.get(key)->size());
      accum += REFLOG_SIZE;
    }
  }

  static void verify(index::radix_index& index) {
    uint64_t accum = 0;
    for (int32_t i = 0; i < 128; i++) {
      byte_string key = byte_string(i * 8);
      reflog const* s = index.get(key);
      size_t size = s->size();
      for (uint64_t j = accum; j < accum + size; j++) {
        ASSERT_EQ(j, s->at(j - accum));
      }
      accum += size;
    }
  }

  static schema_t schema() {
    schema_builder builder;
    builder.add_column(SHORT_TYPE, "a");
    return schema_t(builder.get_columns());
  }

  static void verify_reflog_archived(reflog const* reflog, size_t reflog_archival_tail) {
    for (uint32_t i = 0; i < reflog_archival_tail; i += reflog_constants::BUCKET_SIZE) {
      storage::read_only_encoded_ptr<uint64_t> bucket_ptr;
      reflog->ptr(i, bucket_ptr);
      void* ptr = bucket_ptr.get().ptr();
      auto aux = storage::ptr_aux_block::get(storage::ptr_metadata::get(bucket_ptr.get().ptr()));
      ASSERT_EQ(aux.state_, storage::state_type::D_ARCHIVED);
    }
    for (uint32_t i = reflog_archival_tail; i < reflog->size(); i += reflog_constants::BUCKET_SIZE) {
      storage::read_only_encoded_ptr<uint64_t> bucket_ptr;
      reflog->ptr(i, bucket_ptr);
      void* ptr = bucket_ptr.get().ptr();
      auto aux = storage::ptr_aux_block::get(storage::ptr_metadata::get(bucket_ptr.get().ptr()));
      ASSERT_EQ(aux.state_, storage::state_type::D_IN_MEMORY);
    }
  }

};

const size_t IndexArchivalTest::NUM_BUCKETS_PER_REFLOG;
const size_t IndexArchivalTest::REFLOG_SIZE;

TEST_F(IndexArchivalTest, IndexCorrectnessTest) {

  index::radix_index index(sizeof(int32_t), 256);
  schema_t s = schema();
  fill(index);
  LOG_INFO << "verify1";
  verify(index);

  index_log indexes;
  indexes.push_back(&index);

  std::string path = "/tmp/index_archives/";
  file_utils::clear_dir(path);
  archival::index_log_archiver archiver(path, &indexes, &s);

  verify(index);
  archiver.archive(1 * reflog_constants::BUCKET_SIZE);
  archiver.archive(2 * reflog_constants::BUCKET_SIZE);
  archiver.archive(4 * reflog_constants::BUCKET_SIZE);
  archiver.archive(8 * reflog_constants::BUCKET_SIZE);
  archiver.archive(16 * reflog_constants::BUCKET_SIZE);
  verify(index);
}

#endif /* TEST_INDEX_ARCHIVAL_TEST_H_ */

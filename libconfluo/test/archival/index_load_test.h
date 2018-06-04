#ifndef TEST_ARCHIVAL_INDEX_LOAD_TEST_H_
#define TEST_ARCHIVAL_INDEX_LOAD_TEST_H_

#include <thread>

#include "archival/archival_utils.h"
#include "archival/index_log_archiver.h"
#include "index_log.h"
#include "container/radix_tree.h"
#include "storage/ptr_aux_block.h"
#include "schema/schema.h"
#include "gtest/gtest.h"

using namespace ::confluo;
using namespace ::confluo::archival;

class IndexLoadTest : public testing::Test {
 public:

  static const uint64_t NUM_BUCKETS_PER_REFLOG = 2;
  static const uint64_t REFLOG_SIZE = NUM_BUCKETS_PER_REFLOG * reflog_constants::BUCKET_SIZE;

  static void fill(index::radix_index &index) {
    uint64_t accum = 0;
    for (uint32_t i = 0; i < 128; i++) {
      uint32_t val = i;
      byte_string key = byte_string(val);
      for (uint64_t j = accum; j < accum + REFLOG_SIZE; j++) {
        index.insert(key, j);
      }
      ASSERT_EQ(REFLOG_SIZE, index.get(key)->size());
      accum += REFLOG_SIZE;
    }
  }

  static void verify(index::radix_index &index) {
    uint64_t accum = 0;
    for (uint32_t i = 0; i < 128; i++) {
      uint32_t val = i;
      byte_string key = byte_string(val);
      reflog const *s = index.get(key);
      size_t size = s->size();
      for (uint64_t j = accum; j < accum + size; j++) {
        ASSERT_EQ(j, s->at(j - accum));
      }
      accum += size;
    }
  }

  static schema_t schema() {
    schema_builder builder;
    builder.add_column(primitive_types::UINT_TYPE(), "a");
    return schema_t(builder.get_columns());
  }

  static void verify_reflog_archived(reflog const *reflog, size_t reflog_archival_tail) {
    for (uint32_t i = 0; i < reflog_archival_tail; i += reflog_constants::BUCKET_SIZE) {
      storage::read_only_encoded_ptr<uint64_t> bucket_ptr;
      reflog->ptr(i, bucket_ptr);
      void *ptr = bucket_ptr.get().ptr();
      auto aux = storage::ptr_aux_block::get(storage::ptr_metadata::get(bucket_ptr.get().ptr()));
      ASSERT_EQ(aux.state_, storage::state_type::D_ARCHIVED);
    }
    for (uint32_t i = static_cast<uint32_t>(reflog_archival_tail); i < reflog->size();
         i += reflog_constants::BUCKET_SIZE) {
      storage::read_only_encoded_ptr<uint64_t> bucket_ptr;
      reflog->ptr(i, bucket_ptr);
      void *ptr = bucket_ptr.get().ptr();
      auto aux = storage::ptr_aux_block::get(storage::ptr_metadata::get(bucket_ptr.get().ptr()));
      ASSERT_EQ(aux.state_, storage::state_type::D_IN_MEMORY);
    }
  }

};

const uint64_t IndexLoadTest::NUM_BUCKETS_PER_REFLOG;
const uint64_t IndexLoadTest::REFLOG_SIZE;

TEST_F(IndexLoadTest, IndexLogLoadTest) {

  std::string path = "/tmp/index_archives/";
  file_utils::clear_dir(path);
  size_t index_id;
  schema_t s = schema();
  size_t field_size = s[s.get_field_index("a")].type().size;

  {
    index::radix_index index(field_size, 256);
    fill(index);
    index_log indexes;
    index_id = indexes.push_back(&index);
    s[s.get_field_index("a")].set_indexing();
    s[s.get_field_index("a")].set_indexed(static_cast<uint16_t>(index_id), configuration_params::INDEX_BUCKET_SIZE());

    archival::index_log_archiver archiver(path, &indexes, &s);
    archiver.archive(static_cast<size_t>(1e6));
    verify(index);
  }

  index::radix_index index2(field_size, 256);
  archival::index_load_utils::load(archival_utils::index_archival_path(path, index_id), &index2);
  verify(index2);

}

#endif /* TEST_ARCHIVAL_INDEX_LOAD_TEST_H_ */

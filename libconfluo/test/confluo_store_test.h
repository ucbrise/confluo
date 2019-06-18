#ifndef CONFLUO_TEST_CONFLUO_STORE_TEST_H_
#define CONFLUO_TEST_CONFLUO_STORE_TEST_H_

#include "confluo_store.h"
#include "gtest/gtest.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::confluo;

class ConfluoStoreTest : public testing::Test {
 public:
  static std::vector<column_t> s;

  static std::vector<column_t> schema() {
    schema_builder builder;
    builder.add_column(primitive_types::BOOL_TYPE(), "a");
    builder.add_column(primitive_types::CHAR_TYPE(), "b");
    builder.add_column(primitive_types::SHORT_TYPE(), "c");
    builder.add_column(primitive_types::INT_TYPE(), "d");
    builder.add_column(primitive_types::LONG_TYPE(), "e");
    builder.add_column(primitive_types::FLOAT_TYPE(), "f");
    builder.add_column(primitive_types::DOUBLE_TYPE(), "g");
    builder.add_column(primitive_types::STRING_TYPE(16), "h");
    return builder.get_columns();
  }

};

std::vector<column_t> ConfluoStoreTest::s = schema();

TEST_F(ConfluoStoreTest, AddTableTest) {
  confluo_store store("/tmp");
  int64_t id = store.create_atomic_multilog("my_table", s, storage::storage_mode::IN_MEMORY);
  ASSERT_EQ(id, store.get_atomic_multilog_id("my_table"));
}

TEST_F(ConfluoStoreTest, RemoveTableTest) {
  confluo_store store("/tmp");
  int64_t id = store.create_atomic_multilog("my_table", s, storage::storage_mode::IN_MEMORY);
  ASSERT_NE(-1, store.remove_atomic_multilog(id));
  try {
    store.remove_atomic_multilog("my_table");
  } catch (std::exception &e) {
    ASSERT_STREQ("No such atomic multilog my_table", e.what());
  }
}

TEST_F(ConfluoStoreTest, LoadTest) {
  std::vector<column_t> s = schema();

  confluo_store store("/tmp");
  int64_t id = store.create_atomic_multilog("my_table", s, storage::storage_mode::IN_MEMORY);
  auto *mlog = store.get_atomic_multilog(id);

  typedef std::vector<std::string> rec_vector;
  mlog->append(rec_vector{"false", "0", "0", "0", "0", "0.000000", "0.010000", "abc"});
  mlog->append(rec_vector{"true", "1", "10", "2", "1", "0.100000", "0.020000", "defg"});
  mlog->append(rec_vector{"false", "2", "20", "4", "10", "0.200000", "0.030000", "hijkl"});
  mlog->append(rec_vector{"true", "3", "30", "6", "100", "0.300000", "0.040000", "mnopqr"});
  mlog->append(rec_vector{"false", "4", "40", "8", "1000", "0.400000", "0.050000", "stuvwx"});
  mlog->append(rec_vector{"true", "5", "50", "10", "10000", "0.500000", "0.060000", "yyy"});
  mlog->append(rec_vector{"false", "6", "60", "12", "100000", "0.600000", "0.070000", "zzz"});
  mlog->append(rec_vector{"true", "7", "70", "14", "1000000", "0.700000", "0.080000", "zzz"});

  // Archive data that we will attempt to load
  mlog->archive();
  store.remove_atomic_multilog("my_table");

  // Load data back into a new multilog
  int64_t loaded_mlog_id = store.load_atomic_multilog("my_table");
  auto *loaded_mlog = store.get_atomic_multilog(loaded_mlog_id);

  for (uint64_t i = 0; i < loaded_mlog->num_records() * loaded_mlog->record_size(); i++) {
    ASSERT_EQ(loaded_mlog->read_raw(i) , mlog->read_raw(i));
  }
}

#endif /* CONFLUO_TEST_CONFLUO_STORE_TEST_H_ */

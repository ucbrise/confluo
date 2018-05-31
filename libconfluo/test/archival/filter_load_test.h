#ifndef TEST_ARCHIVAL_LOAD_FILTER_TEST_H_
#define TEST_ARCHIVAL_LOAD_FILTER_TEST_H_

#include <thread>

#include "filter.h"
#include "archival/filter_archiver.h"
#include "gtest/gtest.h"

#include "archival/load_utils.h"
#include "schema/schema.h"
#include "types/type_manager.h"

using namespace ::confluo;

class FilterLoadTest : public testing::Test {
 public:
  typedef archival::filter_log_archiver filter_log_archiver_t;

  static const uint64_t kMaxEntries = static_cast<const uint64_t>(1e6);
  static const uint64_t kTimeBlock = static_cast<const uint64_t>(1e3);
  static const uint64_t kPerTimeBlock = 10;
  static const uint64_t kMillisecs = static_cast<const uint64_t>(1e6);
  static const uint64_t record_size = 16;

  struct data_point {
    int64_t ts;
    int64_t val;

    data_point(uint64_t _ts, int64_t _val)
        : ts(_ts),
          val(_val) {
    }
  }__attribute__((packed));

  void fill(data_log &log, filter &f, size_t start = 0) {
    schema_t schema = build_schema();
    for (size_t i = start; i < start + kMaxEntries / kPerTimeBlock; i++) {
      for (size_t j = i * kPerTimeBlock; j < (i + 1) * kPerTimeBlock; j++) {
        data_point p(i * kTimeBlock, static_cast<int64_t>(j));
        size_t off = log.append(reinterpret_cast<uint8_t *>(&p), schema.record_size());
        record_t r = schema.apply_unsafe(off, reinterpret_cast<uint8_t *>(&p));
        f.update(r);
      }
    }
  }

  schema_t build_schema() {
    return schema_t(schema_builder().add_column(primitive_types::LONG_TYPE(), "long_field").get_columns());
  }

  void verify(filter &f) {
    size_t accum = 0;
    for (size_t t = 0; t < 100; t++) {
      reflog const *s = f.lookup(t);
      size_t size = s->size();
      for (uint32_t i = static_cast<uint32_t>(accum); i < accum + size; i++) {
        ASSERT_EQ(i * 16, s->at(i - accum));
      }
      accum += size;
    }
  }

  static bool filter_none(const record_t &r) {
    return true;
  }

};

TEST_F(FilterLoadTest, FilterLogLoadTest) {
  std::string path = "/tmp/filter_archives/";
  file_utils::clear_dir(path);
  schema_t schema = build_schema();
  data_log log("data_log", "/tmp", storage::IN_MEMORY);

  {
    filter f(filter_none);
    fill(log, f);

    filter_log filters;
    filters.push_back(&f);

    filter_log_archiver_t archiver(path, &filters);
    archiver.archive(kMaxEntries * record_size);
  }

  filter_log filters;
  filter f(filter_none);
  filters.push_back(&f);

  archival::load_utils::load_replay_filter_log(path, filters, log, schema);

  verify(f);
}

TEST_F(FilterLoadTest, FilterReplayTest) {
  schema_t schema = build_schema();
  data_log log("data_log", "/tmp", storage::IN_MEMORY);
  filter f(filter_none);
  fill(log, f);

  filter f_replayed(filter_none);
  archival::load_utils::replay_filter(&f_replayed, log, schema, 0);

  verify(f);
}

TEST_F(FilterLoadTest, FilterLogLoadReplayTest) {
  std::string path = "/tmp/filter_archives/";
  file_utils::clear_dir(path);
  schema_t schema = build_schema();
  data_log log("data_log", "/tmp", storage::IN_MEMORY);

  {
    filter f(filter_none);
    fill(log, f);

    filter_log filters;
    filters.push_back(&f);

    filter_log_archiver_t archiver(path, &filters);
    archiver.archive(kMaxEntries * record_size / 2);
  }

  filter_log filters;
  filter f(filter_none);
  filters.push_back(&f);
  archival::load_utils::load_replay_filter_log(path, filters, log, schema);

  verify(f);
}

TEST_F(FilterLoadTest, FilterLogResumeArchivalAfterLoadTest) {
  std::string path = "/tmp/filter_archives/";
  file_utils::clear_dir(path);
  schema_t schema = build_schema();
  data_log log("data_log", "/tmp", storage::IN_MEMORY);

  {
    filter f(filter_none);
    fill(log, f);

    filter_log filters;
    filters.push_back(&f);

    filter_log_archiver_t archiver(path, &filters);
    archiver.archive(kMaxEntries * record_size);
  }

  {
    filter_log filters;
    filter f(filter_none);
    filters.push_back(&f);
    archival::load_utils::load_replay_filter_log(path, filters, log, schema);

    fill(log, f, kMaxEntries / kPerTimeBlock);

    filter_log_archiver_t archiver(path, &filters);
    archiver.archive(2 * kMaxEntries * record_size);
  }

  filter_log filters;
  filter f(filter_none);
  filters.push_back(&f);
  archival::load_utils::load_replay_filter_log(path, filters, log, schema);
}

#endif /* TEST_ARCHIVAL_LOAD_FILTER_TEST_H_ */

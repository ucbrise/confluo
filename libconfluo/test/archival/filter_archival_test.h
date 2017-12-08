#ifndef TEST_FILTER_ARCHIVAL_TEST_H_
#define TEST_FILTER_ARCHIVAL_TEST_H_

#include <thread>

#include "storage/encoder.h"
#include "filter.h"
#include "archival/filter_archiver.h"
#include "gtest/gtest.h"

using namespace ::confluo;

class FilterArchivalTest : public testing::Test {
 public:
  typedef archival::filter_log_archiver<archival::encoding_type::IDENTITY> filter_log_archiver_t;

  static const uint64_t kMaxEntries = 1e6;
  static const uint64_t kTimeBlock = 1e3;
  static const uint64_t kPerTimeBlock = 10;
  static const uint64_t kMillisecs = 1e6;

  struct data_point {
    int64_t ts;
    int64_t val;

    data_point(uint64_t _ts, int64_t _val)
        : ts(_ts),
          val(_val) {
    }
  }__attribute__((packed));

  void fill(filter& f) {
    ASSERT_TRUE(thread_manager::register_thread() != -1);
    for (size_t i = 0; i < kMaxEntries / kPerTimeBlock; i++) {
      for (size_t j = i * kPerTimeBlock; j < (i + 1) * kPerTimeBlock; j++) {
        data_point p(i * kTimeBlock, j);
        record_t r(j, reinterpret_cast<uint8_t*>(&p), sizeof(data_point));
        r.push_back(field_t(0, LONG_TYPE, r.data(), false, 0, 0.0));
        r.push_back(
            field_t(1, LONG_TYPE,
                    reinterpret_cast<char*>(r.data()) + sizeof(int64_t), false, 0,
                    0.0));
        f.update(r);
      }
    }
    ASSERT_TRUE(thread_manager::deregister_thread() != -1);
  }

  void verify(filter& f) {
    size_t accum = 0;
    for (size_t t = 0; t < 100; t++) {
      reflog const* s = f.lookup(t);
      size_t size = s->size();
      ASSERT_EQ(static_cast<size_t>(10000), size);
      for (uint32_t i = accum; i < accum + size; i++) {
        ASSERT_EQ(i, s->at(i - accum));
      }
      accum += size;
    }
  }

  void verify_reflog_archived(reflog const* reflog, size_t reflog_archival_tail) {
    for (uint32_t i = 0; i < reflog_archival_tail; i += reflog_constants::BUCKET_SIZE) {
      storage::read_only_encoded_ptr<uint64_t> bucket_ptr;
      reflog->ptr(i, bucket_ptr);
      void* ptr = bucket_ptr.get().ptr();
      ASSERT_EQ(storage::ptr_metadata::get(ptr)->state_, storage::state_type::D_ARCHIVED);
    }
    for (uint32_t i = reflog_archival_tail; i < reflog->size(); i += reflog_constants::BUCKET_SIZE) {
      storage::read_only_encoded_ptr<uint64_t> bucket_ptr;
      reflog->ptr(i, bucket_ptr);
      void* ptr = bucket_ptr.get().ptr();
      ASSERT_EQ(storage::ptr_metadata::get(ptr)->state_, storage::state_type::D_IN_MEMORY);
    }
  }

  static bool filter_none(const record_t& r) {
    return true;
  }

};

TEST_F(FilterArchivalTest, FilterCorrectnessTestSingleCall) {
  read_tail rt("/tmp", storage::IN_MEMORY);

  filter f(filter_none);
  fill(f);

  filter_log filters;
  filters.push_back(&f);
  filter_log_archiver_t archiver("/tmp/filter_archives/", filters);

  reflog const* first_reflog = f.lookup(0);

  verify(f);
  archiver.archive(32768);
  verify(f);

//  verify_reflog_archived(first_reflog, 0);
}

TEST_F(FilterArchivalTest, FilterCorrectnessTestMultipleCalls) {
  read_tail rt("/tmp", storage::IN_MEMORY);

  filter f(filter_none);
  fill(f);

  filter_log filters;
  filters.push_back(&f);
  filter_log_archiver_t archiver("/tmp/filter_archives/", filters);

  reflog const* first_reflog = f.lookup(0);

  verify(f);
  archiver.archive(1000);
  archiver.archive(2000);
  archiver.archive(4000);
  archiver.archive(8000);
  archiver.archive(16000);
  archiver.archive(32000);
  verify(f);
}

TEST_F(FilterArchivalTest, FirstReflogArchivedTest) {
  read_tail rt("/tmp", storage::IN_MEMORY);

  filter f(filter_none);
  fill(f);

  filter_log filters;
  filters.push_back(&f);
  filter_log_archiver_t archiver("/tmp/filter_archives/", filters);

  reflog const* first_reflog = f.lookup(0);
  reflog const* second_reflog = f.lookup(1);

  verify_reflog_archived(first_reflog, 0);

  // archive nothing
  archiver.archive(1000);
  verify_reflog_archived(first_reflog, 0);

  // archives only the first bucket of the first reflog
  archiver.archive(2000);
  verify_reflog_archived(first_reflog, reflog_constants::BUCKET_SIZE);

  // archives the second bucket of the first reflog
  archiver.archive(4000);
  verify_reflog_archived(first_reflog, 3 * reflog_constants::BUCKET_SIZE);

  // archive multiple buckets in the first reflog
  archiver.archive(8000);
  verify_reflog_archived(first_reflog, 7 * reflog_constants::BUCKET_SIZE);

  // archive across contiguous reflogs
  archiver.archive(20000);
  verify_reflog_archived(first_reflog, 10 * reflog_constants::BUCKET_SIZE);
  verify_reflog_archived(second_reflog, 10 * reflog_constants::BUCKET_SIZE);
}

TEST_F(FilterArchivalTest, MultipleReflogsArchivedTest) {
  read_tail rt("/tmp", storage::IN_MEMORY);

  filter f(filter_none);
  fill(f);

  filter_log filters;
  filters.push_back(&f);
  filter_log_archiver_t archiver("/tmp/filter_archives/", filters);

  // archive across contiguous reflogs
  archiver.archive(32000);
  verify_reflog_archived(f.lookup(0), 10 * reflog_constants::BUCKET_SIZE);
  verify_reflog_archived(f.lookup(1), 10 * reflog_constants::BUCKET_SIZE);
  verify_reflog_archived(f.lookup(2), 10 * reflog_constants::BUCKET_SIZE);
  verify_reflog_archived(f.lookup(3), 1 * reflog_constants::BUCKET_SIZE);

  verify(f);
}

#endif /* TEST_FILTER_ARCHIVAL_TEST_H_ */

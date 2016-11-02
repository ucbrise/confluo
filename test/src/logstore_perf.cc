#include "logstore.h"
#include "test_utils.h"
#include "gtest/gtest.h"

#include <ctime>
#include <chrono>
#include <fstream>

using namespace ::std::chrono;

std::string res_path_logstore;

class LogStorePerf : public testing::Test {
 public:
  class DummyGenerator {
   public:
    DummyGenerator() {
    }

    static void gentokens(slog::token_list& tokens,
                          const std::vector<uint32_t>& index_ids,
                          const uint32_t val) {
      for (uint32_t i = 0; i < index_ids.size(); i++) {
        slog::token_t tok;
        tok.index_id = index_ids[i];
        if (index_ids[i] == 1024 || index_ids[i] == 2048) {
          tok.len = 4;
        } else if (index_ids[i] == 4096 || index_ids[i] == 8192) {
          tok.len = 3;
        } else if (index_ids[i] == 16384) {
          tok.len = 2;
        } else if (index_ids[i] == 32768) {
          tok.len = 1;
        }
        tok.data = new unsigned char[tok.len];
        gentok(tok.data, tok.len, val);
        tokens.push_back(tok);
      }
    }

    static void gentok(unsigned char* tok, uint32_t len, uint32_t val) {
      for (uint32_t i = 0; i < len; i++) {
        tok[i] = val;
      }
    }
  };

  LogStorePerf() {
    res.open(res_path_logstore, std::fstream::app);
  }

  ~LogStorePerf() {
  }

  void add_and_check_indexes(slog::log_store& ls,
                             std::vector<uint32_t>& index_ids) {
    index_ids.push_back(ls.add_index(4, 3));
    index_ids.push_back(ls.add_index(4, 2));
    index_ids.push_back(ls.add_index(3, 3));
    index_ids.push_back(ls.add_index(3, 2));
    index_ids.push_back(ls.add_index(2, 2));
    index_ids.push_back(ls.add_index(1, 1));

    uint32_t id = 1024;
    for (uint32_t i = 0; i < index_ids.size(); i++) {
      ASSERT_EQ(index_ids[i], id);
      id *= 2;
    }
  }

  void add_and_check_streams(slog::log_store & ls,
                             std::vector<uint32_t>& stream_ids) {
    stream_ids.push_back(ls.add_stream(filter_fn1));
    stream_ids.push_back(ls.add_stream(filter_fn2));

    ASSERT_EQ(stream_ids[0], 0);
    ASSERT_EQ(stream_ids[1], 1);
  }

  std::array<slog::token_list, 256> generate_token_lists(
      const std::vector<uint32_t>& index_ids) {
    std::array<slog::token_list, 256> token_lists;
    for (uint32_t i = 0; i < 256; i++) {
      LogStorePerf::DummyGenerator::gentokens(token_lists[i], index_ids, i);
    }
    return token_lists;
  }

  std::array<slog::filter_query, 1536> generate_queries(
      std::array<slog::token_list, 256>& token_lists) {
    std::array<slog::filter_query, 1536> queries;

    uint32_t query_id = 0;
    for (uint32_t i = 0; i < 256; i++) {
      for (uint32_t j = 0; j < 6; j++) {
        slog::filter_conjunction conjunction;
        slog::basic_filter f;
        f.index_id = token_lists[i][j].index_id;
        f.token_prefix = token_lists[i][j].data;
        f.token_prefix_len = token_lists[i][j].len;
        conjunction.push_back(f);
        queries[query_id].push_back(conjunction);
        query_id++;
      }
    }
    return queries;
  }

 protected:
  unsigned char hdr[40];
  std::ofstream res;
};

TEST_F(LogStorePerf, InsertAndGetPerf) {
  slog::log_store ls;
  std::vector<uint32_t> index_ids;
  std::vector<uint32_t> stream_ids;

  add_and_check_indexes(ls, index_ids);
  add_and_check_streams(ls, stream_ids);

  auto tokens = generate_token_lists(index_ids);

  LogStorePerf::DummyGenerator::gentok(hdr, 40, 256);
  auto write_start = high_resolution_clock::now();
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(hdr, 40, tokens[i % 256]);
  }
  auto write_end = high_resolution_clock::now();
  double write_time =
      duration_cast<microseconds>(write_end - write_start).count();

  unsigned char ret[40];
  auto read_start = high_resolution_clock::now();
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    bool success = ls.get(ret, i);
    ASSERT_TRUE(success);
  }
  auto read_end = high_resolution_clock::now();
  double read_time = duration_cast<microseconds>(read_end - read_start).count();

  time_t tt = system_clock::to_time_t(system_clock::now());
  res << "indexandget" << "\t" << (write_time / kMaxKeys) << "\t"
      << (read_time / kMaxKeys) << "\n";
}

TEST_F(LogStorePerf, InsertAndFilterPerf) {
  slog::log_store ls;
  std::vector<uint32_t> index_ids;
  std::vector<uint32_t> stream_ids;

  add_and_check_indexes(ls, index_ids);
  add_and_check_streams(ls, stream_ids);

  auto token_lists = generate_token_lists(index_ids);

  LogStorePerf::DummyGenerator::gentok(hdr, 40, 256);
  auto write_start = high_resolution_clock::now();
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(hdr, 40, token_lists[i % 256]);
  }
  auto write_end = high_resolution_clock::now();
  double write_time =
      duration_cast<microseconds>(write_end - write_start).count();

  auto queries = generate_queries(token_lists);
  auto read_start = high_resolution_clock::now();
  for (uint32_t i = 0; i < queries.size(); i++) {
    std::unordered_set<uint64_t> results;
    ls.filter(results, queries[i]);
    ASSERT_EQ(results.size(), 10);
  }
  auto read_end = high_resolution_clock::now();
  double read_time = duration_cast<microseconds>(read_end - read_start).count();

  time_t tt = system_clock::to_time_t(system_clock::now());
  res << "indexandfilter" << "\t" << (write_time / kMaxKeys) << "\t"
      << (read_time / queries.size()) << "\n";
}

TEST_F(LogStorePerf, InsertAndStreamPerf) {
  slog::log_store ls;
  std::vector<uint32_t> index_ids;
  std::vector<uint32_t> stream_ids;

  add_and_check_indexes(ls, index_ids);
  add_and_check_streams(ls, stream_ids);

  auto token_lists = generate_token_lists(index_ids);

  LogStorePerf::DummyGenerator::gentok(hdr, 40, 256);
  auto write_start = high_resolution_clock::now();
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(hdr, 40, token_lists[i % 256]);
  }
  auto write_end = high_resolution_clock::now();
  double write_time =
      duration_cast<microseconds>(write_end - write_start).count();

  slog::entry_list* stream1 = ls.get_stream(0);
  ASSERT_EQ(stream1->size(), kMaxKeys / 10);
  auto read_start1 = high_resolution_clock::now();
  for (uint32_t i = 0; i < stream1->size(); i++) {
    ASSERT_TRUE(stream1->at(i) % 10 == 0);
    ASSERT_EQ(stream1->at(i) / 10, i);
  }
  auto read_end1 = high_resolution_clock::now();
  double read_time1 =
      duration_cast<microseconds>(read_end1 - read_start1).count();

  slog::entry_list* stream2 = ls.get_stream(1);
  ASSERT_EQ(stream2->size(), kMaxKeys / 10);
  auto read_start2 = high_resolution_clock::now();
  for (uint32_t i = 0; i < stream2->size(); i++) {
    ASSERT_EQ(stream2->at(i), i);
  }
  auto read_end2 = high_resolution_clock::now();
  double read_time2 =
      duration_cast<microseconds>(read_end2 - read_start2).count();
  time_t tt = system_clock::to_time_t(system_clock::now());
  res << "indexandstream1" << "\t" << (write_time / kMaxKeys) << "\t"
      << read_time1 << "\n";
  res << "indexandstream2" << "\t" << (write_time / kMaxKeys) << "\t"
      << read_time2 << "\n";
}

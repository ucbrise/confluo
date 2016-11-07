#include "logstore.h"
#include "test_utils.h"
#include "gtest/gtest.h"

class LogStoreTest : public testing::Test {
 public:
  class DummyGenerator {
   public:
    DummyGenerator() {
    }

    static void gentokens(slog::token_list& tokens,
                          const std::vector<uint32_t>& index_ids,
                          const uint32_t val) {
      for (uint32_t i = 0; i < index_ids.size(); i++) {
        tokens.push_back(slog::token_t(index_ids[i], val));
      }
    }

    static void gendata(unsigned char* buf, uint32_t len, uint32_t val) {
      for (uint32_t i = 0; i < len; i++) {
        buf[i] = val;
      }
    }
  };

  LogStoreTest() {
  }

  ~LogStoreTest() {
  }

  void add_and_check_indexes(slog::log_store& ls,
                             std::vector<uint32_t>& index_ids) {

    for (uint32_t i = 1; i <= 8; i++) {
      index_ids.push_back(ls.add_index(i));
    }

    uint32_t id = 1024;
    for (uint32_t i = 0; i < index_ids.size(); i++) {
      ASSERT_EQ(id, index_ids[i]);
      id *= 2;
    }
  }

  void add_and_check_streams(slog::log_store & ls,
                             std::vector<uint32_t>& stream_ids) {
    stream_ids.push_back(ls.add_stream(filter_fn1));
    stream_ids.push_back(ls.add_stream(filter_fn2));

    ASSERT_EQ(stream_ids[0], 0U);
    ASSERT_EQ(stream_ids[1], 1U);
  }

  std::array<slog::token_list, 256> generate_token_lists(
      const std::vector<uint32_t>& index_ids) {
    std::array<slog::token_list, 256> token_lists;
    for (uint32_t i = 0; i < 256; i++) {
      LogStoreTest::DummyGenerator::gentokens(token_lists[i], index_ids, i);
    }
    return token_lists;
  }

  std::array<slog::filter_query, 2048> generate_queries(
      std::array<slog::token_list, 256>& token_lists) {
    std::array<slog::filter_query, 2048> queries;

    uint32_t query_id = 0;
    for (uint32_t i = 0; i < 256; i++) {
      for (uint32_t j = 0; j < 8; j++) {
        slog::filter_conjunction conjunction;
        conjunction.push_back(
            slog::basic_filter(token_lists[i][j].index_id(),
                               token_lists[i][j].data()));
        queries[query_id].push_back(conjunction);
        query_id++;
      }
    }
    return queries;
  }

 protected:
  unsigned char hdr[40];
};

TEST_F(LogStoreTest, InsertAndGetTest) {
  slog::log_store ls;
  std::vector<uint32_t> index_ids;
  std::vector<uint32_t> stream_ids;

  add_and_check_indexes(ls, index_ids);
  add_and_check_streams(ls, stream_ids);

  auto tokens = generate_token_lists(index_ids);

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    LogStoreTest::DummyGenerator::gendata(hdr, 40, i);
    ls.insert(hdr, 40, tokens[i % 256]);
  }

  unsigned char ret[40];
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    bool success = ls.get(ret, i);
    ASSERT_TRUE(success);
    for (uint32_t j = 0; j < 40; j++) {
      unsigned char expected = i % 256;
      ASSERT_EQ(ret[j], expected);
    }
  }
}

TEST_F(LogStoreTest, InsertAndFilterTest) {
  slog::log_store ls;
  std::vector<uint32_t> index_ids;
  std::vector<uint32_t> stream_ids;

  add_and_check_indexes(ls, index_ids);
  add_and_check_streams(ls, stream_ids);

  auto token_lists = generate_token_lists(index_ids);

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    LogStoreTest::DummyGenerator::gendata(hdr, 40, i);
    ls.insert(hdr, 40, token_lists[i % 256]);
  }

  auto queries = generate_queries(token_lists);
  for (uint32_t i = 0; i < queries.size(); i++) {
    std::unordered_set<uint64_t> results;
    ls.filter(results, queries[i]);
    ASSERT_EQ(results.size(), static_cast<size_t>(10));
    for (uint64_t id : results) {
      ASSERT_EQ(id % 256, i / 8);
    }
    results.clear();
  }
}

TEST_F(LogStoreTest, InsertAndStreamTest) {
  slog::log_store ls;
  std::vector<uint32_t> index_ids;
  std::vector<uint32_t> stream_ids;

  add_and_check_indexes(ls, index_ids);
  add_and_check_streams(ls, stream_ids);

  auto token_lists = generate_token_lists(index_ids);

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    LogStoreTest::DummyGenerator::gendata(hdr, 40, i);
    ls.insert(hdr, 40, token_lists[i % 256]);
  }

  slog::entry_list* stream1 = ls.get_stream(0);
  ASSERT_EQ(stream1->size(), kMaxKeys / 10);

  for (uint32_t i = 0; i < stream1->size(); i++) {
    ASSERT_TRUE(stream1->at(i) % 10 == 0);
    ASSERT_EQ(stream1->at(i) / 10, i);
  }

  slog::entry_list* stream2 = ls.get_stream(1);
  ASSERT_EQ(stream2->size(), kMaxKeys / 10);
  for (uint32_t i = 0; i < stream2->size(); i++) {
    ASSERT_EQ(stream2->at(i), i);
  }
}

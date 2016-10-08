#include "logstore.h"

#include "gtest/gtest.h"

#define kMaxKeys 2560

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

bool filter_fn1(uint64_t& record_id, const unsigned char* record,
                const uint16_t record_len, const slog::token_list& list) {
  return record_id % 10 == 0;
}

bool filter_fn2(uint64_t& record_id, const unsigned char* record,
                const uint16_t record_len, const slog::token_list& list) {
  bool filter = record_id % 10 == 0;
  record_id /= 10;
  return filter;
}

class LogStoreTest : public testing::Test {
 public:
  LogStoreTest() {
  }

  ~LogStoreTest() {
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

  std::array<slog::token_list, 256> generate_tokens(
      const std::vector<uint32_t>& index_ids) {
    std::array<slog::token_list, 256> token_lists;
    for (uint32_t i = 0; i < 256; i++) {
      DummyGenerator::gentokens(token_lists[i], index_ids, i);
    }
    return token_lists;
  }

  std::array<slog::filter_query, 1536> generate_queries(
      std::array<slog::token_list, 256>& token_lists) {
    std::array<slog::filter_query, 1536> queries;
    for (uint32_t i = 0; i < 256; i++) {
      for (uint32_t j = 0; j < 6; j++) {
        slog::filter_conjunction conjunction;
        slog::basic_filter f;
        f.index_id = token_lists[i][j].index_id;
        f.token_prefix = token_lists[i][j].data;
        f.token_prefix_len = token_lists[i][j].len;
        conjunction.push_back(f);
        queries[i].push_back(conjunction);
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

  auto tokens = generate_tokens(index_ids);

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    DummyGenerator::gentok(hdr, 40, i);
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

  auto token_lists = generate_tokens(index_ids);

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    DummyGenerator::gentok(hdr, 40, i);
    ls.insert(hdr, 40, token_lists[i % 256]);
  }

  auto queries = generate_queries(token_lists);
  for (uint32_t i = 0; i < queries.size(); i++) {
    std::unordered_set<uint64_t> results;
    ls.filter(results, queries[i]);
    ASSERT_EQ(results.size(), 10);
    for (uint64_t id : results) {
      ASSERT_EQ(id % 256, i / 6);
    }
    results.clear();
  }
}

#include "logstore.h"

#include "gtest/gtest.h"

#define kMaxKeys 2560

class DummyGenerator {
 public:
  DummyGenerator() {
  }

  static void alloctokens(slog::tokens& tkns) {
    tkns.time = new unsigned char[4];
    tkns.src_ip = new unsigned char[4];
    tkns.dst_ip = new unsigned char[4];
    tkns.src_prt = new unsigned char[2];
    tkns.dst_prt = new unsigned char[2];
  }

  static void freetokens(slog::tokens& tkns) {
    delete[] tkns.time;
    delete[] tkns.src_ip;
    delete[] tkns.dst_ip;
    delete[] tkns.src_prt;
    delete[] tkns.dst_prt;
  }

  static void gentokens(slog::tokens& tkns, uint64_t val) {
    gentok(tkns.time, 4, val);
    gentok(tkns.src_ip, 4, val);
    gentok(tkns.dst_ip, 4, val);
    gentok(tkns.src_prt, 2, val);
    gentok(tkns.dst_prt, 2, val);
  }

  static void gentok(unsigned char* tok, uint32_t len, uint32_t val) {
    for (uint32_t i = 0; i < len; i++) {
      tok[i] = val;
    }
  }

 private:
};

class LogStoreTest : public testing::Test {
 public:
  LogStoreTest() {
    DummyGenerator::alloctokens(tkns);
  }

  ~LogStoreTest() {
    DummyGenerator::freetokens(tkns);
  }

  unsigned char hdr[40];
  slog::tokens tkns;
};

TEST_F(LogStoreTest, InsertAndGetTest) {
  slog::log_store ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    DummyGenerator::gentok(hdr, 40, i);
    DummyGenerator::gentokens(tkns, i);
    ls.insert(hdr, 40, tkns);
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

TEST_F(LogStoreTest, HandleInsertAndGetTest) {
  slog::log_store ls_orig;
  slog::log_store::handle* ls = ls_orig.get_handle();
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    DummyGenerator::gentok(hdr, 40, i);
    DummyGenerator::gentokens(tkns, i);
    ls->insert(hdr, 40, tkns);
  }

  unsigned char ret[40];
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    bool success = ls->get(ret, i);
    ASSERT_TRUE(success);
    for (uint32_t j = 0; j < 40; j++) {
      unsigned char expected = i % 256;
      ASSERT_EQ(ret[j], expected);
    }
  }
}

TEST_F(LogStoreTest, InsertAndFilterTest) {
  slog::log_store ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    DummyGenerator::gentok(hdr, 40, i);
    DummyGenerator::gentokens(tkns, i);
    ls.insert(hdr, 40, tkns);
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<uint64_t> results;
    DummyGenerator::gentokens(tkns, i);
    ls.filter_src_ip(results, tkns.src_ip, 4);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls.filter_src_ip(results, tkns.src_ip, 3);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls.filter_src_ip(results, tkns.src_ip, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<uint64_t> results;
    DummyGenerator::gentokens(tkns, i);
    ls.filter_dst_ip(results, tkns.dst_ip, 4);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls.filter_dst_ip(results, tkns.dst_ip, 3);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls.filter_dst_ip(results, tkns.dst_ip, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<uint64_t> results;
    DummyGenerator::gentokens(tkns, i);
    ls.filter_src_port(results, tkns.src_prt, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls.filter_src_port(results, tkns.src_prt, 1);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<uint64_t> results;
    DummyGenerator::gentokens(tkns, i);
    ls.filter_dst_port(results, tkns.dst_prt, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls.filter_dst_port(results, tkns.dst_prt, 1);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<uint64_t> results;
    DummyGenerator::gentokens(tkns, i);
    ls.filter_time(results, tkns.time, 4);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls.filter_time(results, tkns.time, 3);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls.filter_time(results, tkns.time, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }
}

TEST_F(LogStoreTest, HandleInsertAndFilterTest) {
  slog::log_store ls_orig;
  slog::log_store::handle* ls = ls_orig.get_handle();
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    DummyGenerator::gentok(hdr, 40, i);
    DummyGenerator::gentokens(tkns, i);
    ls->insert(hdr, 40, tkns);
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<uint64_t> results;
    DummyGenerator::gentokens(tkns, i);
    ls->filter_src_ip(results, tkns.src_ip, 4);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls->filter_src_ip(results, tkns.src_ip, 3);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls->filter_src_ip(results, tkns.src_ip, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<uint64_t> results;
    DummyGenerator::gentokens(tkns, i);
    ls->filter_dst_ip(results, tkns.dst_ip, 4);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls->filter_dst_ip(results, tkns.dst_ip, 3);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls->filter_dst_ip(results, tkns.dst_ip, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<uint64_t> results;
    DummyGenerator::gentokens(tkns, i);
    ls->filter_src_port(results, tkns.src_prt, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls->filter_src_port(results, tkns.src_prt, 1);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<uint64_t> results;
    DummyGenerator::gentokens(tkns, i);
    ls->filter_dst_port(results, tkns.dst_prt, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls->filter_dst_port(results, tkns.dst_prt, 1);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<uint64_t> results;
    DummyGenerator::gentokens(tkns, i);
    ls->filter_time(results, tkns.time, 4);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls->filter_time(results, tkns.time, 3);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();

    ls->filter_time(results, tkns.time, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }
}

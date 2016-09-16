#include "logstore.h"

#include "gtest/gtest.h"

#define kLogStoreSize 4194304
#define kMaxKeys 2560

class DummyGenerator {
 public:
  DummyGenerator() {
  }

  static slog::tokens gentokens(uint64_t val) {
    slog::tokens tkns;
    ipgen(tkns.src_ip, val);
    ipgen(tkns.dst_ip, val);
    portgen(tkns.src_prt, val);
    portgen(tkns.dst_prt, val);
    protgen(tkns.prot, val);
    return tkns;
  }

  static void hdrgen(unsigned char* hdr, uint64_t val) {
    for (uint32_t i = 0; i < 40; i++) {
      hdr[i] = val % 256;
    }
  }

 private:
  static void ipgen(unsigned char* ip, uint32_t val) {
    ip[0] = val % 256;
    ip[1] = val % 256;
    ip[2] = val % 256;
    ip[3] = val % 256;
  }

  static void portgen(unsigned char* port, uint32_t val) {
    port[0] = val % 256;
    port[1] = val % 256;
  }

  static void protgen(unsigned char* prot, uint32_t val) {
    prot[0] = val % 256;
  }
};

class LogStoreTest : public testing::Test {
 public:
  std::string to_string(uint64_t i) {
    char buf[6];
    sprintf(buf, "%05d", i);
    return std::string(buf);
  }

  unsigned char hdr[40];
};

TEST_F(LogStoreTest, InsertAndGetTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    DummyGenerator::hdrgen(hdr, i);
    ls.insert(hdr, 40, DummyGenerator::gentokens(i));
  }

  unsigned char ret[40];
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.get(ret, i);
    for (uint32_t j = 0; j < 40; j++) {
      unsigned char expected = i % 256;
      ASSERT_EQ(ret[j], expected);
    }
  }
}

TEST_F(LogStoreTest, InsertAndFilterTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    DummyGenerator::hdrgen(hdr, i);
    ls.insert(hdr, 40, DummyGenerator::gentokens(i));
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<int64_t> results;
    slog::tokens tkns = DummyGenerator::gentokens(i);
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
    std::set<int64_t> results;
    slog::tokens tkns = DummyGenerator::gentokens(i);
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
    std::set<int64_t> results;
    slog::tokens tkns = DummyGenerator::gentokens(i);
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
    std::set<int64_t> results;
    slog::tokens tkns = DummyGenerator::gentokens(i);
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
    std::set<int64_t> results;
    slog::tokens tkns = DummyGenerator::gentokens(i);
    ls.filter_prot(results, tkns.prot, 2);
    ASSERT_EQ(results.size(), 10);
    for (int64_t id : results) {
      ASSERT_EQ(id % 256, i % 256);
    }
    results.clear();
  }
}

TEST_F(LogStoreTest, InsertDeleteGetTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    DummyGenerator::hdrgen(hdr, i);
    ls.insert(hdr, 40, DummyGenerator::gentokens(i));
  }

  unsigned char ret[40];
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    bool exists = ls.get(ret, i);
    ASSERT_TRUE(exists);
    for (uint32_t j = 0; j < 40; j++) {
      unsigned char expected = i % 256;
      ASSERT_EQ(ret[j], expected);
    }

    bool deleted = ls.delete_record(i);
    ASSERT_TRUE(deleted);

    exists = ls.get(ret, i);
    ASSERT_FALSE(exists);
  }
}

TEST_F(LogStoreTest, InsertDeleteSearchTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    DummyGenerator::hdrgen(hdr, i);
    ls.insert(hdr, 40, DummyGenerator::gentokens(i));
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<int64_t> results;
    slog::tokens tkns = DummyGenerator::gentokens(i);
    ls.filter_dst_ip(results, tkns.dst_ip, 4);
    uint32_t size_before_delete = results.size();
    results.clear();

    bool success = ls.delete_record(i);
    ASSERT_TRUE(success);

    ls.filter_dst_ip(results, tkns.dst_ip, 4);
    uint32_t size_after_delete = results.size();
    results.clear();
    ASSERT_EQ(size_after_delete, size_before_delete - 1);
  }
}

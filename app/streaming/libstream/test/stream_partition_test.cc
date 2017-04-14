#define GTEST_HAS_TR1_TUPLE 0

#include "stream_partition.h"
#include "gtest/gtest.h"

#include <thread>

using namespace streaming;

class StreamPartitionTest : public testing::Test {
 public:
  static const size_t kDataSize = 64;
  static const uint64_t kBatchSize = 1000;
  static const uint64_t kMax = 10000;

  std::string get_data(uint64_t i) {
    char c = i % 256;
    return std::string(kDataSize, c);
  }

  void check(std::string& data, uint64_t n) {
    ASSERT_EQ(kDataSize, data.length());
    for (size_t i = 0; i < data.length(); i++) {
      ASSERT_EQ(static_cast<const char>(n % 256), data[i]);
    }
  }
};

const uint64_t StreamPartitionTest::kBatchSize;
const uint64_t StreamPartitionTest::kMax;
const size_t StreamPartitionTest::kDataSize;

TEST_F(StreamPartitionTest, WriteReadTest) {
  stream_partition p("/tmp/part.dat");
  for (uint64_t i = 0; i < kMax; i++) {
    p.write(get_data(i));
  }

  for (uint64_t i = 0; i < kMax; i++) {
    std::string data;
    p.read(data, i * kDataSize, kDataSize);
    check(data, i);
  }
}

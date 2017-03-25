#include "log_stream.h"
#include "gtest/gtest.h"

#include <thread>

using namespace ::datastore::stream;

// Stateless filter
struct filter1 {
  bool operator()(uint64_t id, uint8_t* data, size_t len) {
    return id % 10 == 0;
  }

  template<typename T>
  bool operator()(uint64_t id, const T& data) {
    return id % 10 == 0;
  }
};

// Stateful filter
struct filter2 {
  static constexpr double sampling_rate = 0.1;

  filter2()
      : count(0),
        sample_mod(1 / sampling_rate) {
  }

  filter2(const filter2& other)
      : count(atomic::load(&other.count)),
        sample_mod(other.sample_mod) {
  }

  bool operator()(uint64_t id, uint8_t* data, size_t len) {
    return atomic::faa(&count, UINT64_C(1)) % sample_mod == 0;
  }

  template<typename T>
  bool operator()(uint64_t id, const T& data) {
    return atomic::faa(&count, UINT64_C(1)) % sample_mod == 0;
  }

 private:
  atomic::type<uint64_t> count;
  uint64_t sample_mod;
};

class StreamTest : public testing::Test {
 public:
  const uint32_t kMaxEntries = 100000;

  template<typename filter>
  void fill_stream(log_stream<filter>& stream) {
    for (uint32_t i = 0; i < kMaxEntries; i++) {
      stream.update(i, (uint8_t*) &i, sizeof(uint32_t));
    }
  }

  template<typename filter>
  void fill_stream_mt(log_stream<filter>& stream, uint32_t num_threads) {
    std::vector<std::thread> workers;
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::thread([i, &stream, this] {
        for (uint32_t j = (i - 1) * kMaxEntries; j < i * kMaxEntries; j++) {
          stream.update(j, (uint8_t*) &j, sizeof(uint32_t));
        }
      }));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }
  }
};

TEST_F(StreamTest, StreamAddFetchTest1) {
  log_stream<filter1> stream;
  fill_stream(stream);

  const stream_type& s = stream.cstream();
  uint32_t size = s.size();
  ASSERT_EQ(10000U, size);
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i * 10, s.at(i));
  }

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    log_stream<filter1> stream;
    fill_stream_mt(stream, num_threads);

    const stream_type& s = stream.cstream();
    uint32_t size = s.size();
    ASSERT_EQ(10000U * num_threads, size);
    std::vector<uint32_t> counts(size);
    for (uint32_t i = 0; i < size; i++) {
      uint64_t val = s.at(i);
      ASSERT_TRUE(val % 10 == 0);
      counts[val / 10]++;
    }

    for (uint32_t count : counts) {
      ASSERT_EQ(1U, count);
    }
  }
}

TEST_F(StreamTest, StreamAddFetchTest2) {
  log_stream<filter2> stream;
  fill_stream(stream);

  const stream_type& list = stream.cstream();
  uint32_t size = list.size();
  ASSERT_EQ(10000U, size);
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i * 10, list.at(i));
  }

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    log_stream<filter2> stream;
    fill_stream_mt(stream, num_threads);

    const stream_type& list = stream.cstream();
    uint32_t size = list.size();
    ASSERT_EQ(10000U * num_threads, size);
    for (uint32_t i = 0; i < size; i++) {
      uint64_t val = list.at(i);
      ASSERT_TRUE(val < num_threads * kMaxEntries);
    }
  }
}

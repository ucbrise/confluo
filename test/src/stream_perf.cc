#include "streamlog.h"
#include "gtest/gtest.h"

#include "test_utils.h"

using namespace ::std::chrono;

std::string res_path_stream;

class StreamPerf : public testing::Test {
 public:
  StreamPerf() {
    res.open(res_path_stream, std::fstream::app);
  }

  void fill_stream(slog::streamlog& stream) {
    for (uint32_t i = 0; i < kMaxEntries; i++) {
      stream.check_and_add(i, (unsigned char*) &i, sizeof(uint32_t), tokens);
    }
  }

 protected:
  const uint32_t kMaxEntries = 100000;
  std::ofstream res;
  slog::token_list tokens;
};

TEST_F(StreamPerf, StreamAddFetchPerf1) {
  slog::streamlog stream(filter_fn1);

  auto write_start = high_resolution_clock::now();
  fill_stream(stream);
  auto write_end = high_resolution_clock::now();
  double write_time =
      duration_cast<microseconds>(write_end - write_start).count();

  slog::entry_list* list = stream.get_stream();
  uint32_t size = list->size();
  ASSERT_EQ(10000, size);

  auto read_start = high_resolution_clock::now();
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i * 10, list->at(i));
  }
  auto read_end = high_resolution_clock::now();
  double read_time = duration_cast<microseconds>(read_end - read_start).count();

  time_t tt = system_clock::to_time_t(system_clock::now());
  res << "filter_fn1\t" << (write_time / kMaxEntries) << "\t" << read_time
      << std::endl;
}

TEST_F(StreamPerf, StreamAddFetchPerf2) {
  slog::streamlog stream(filter_fn2);

  auto write_start = high_resolution_clock::now();
  fill_stream(stream);
  auto write_end = high_resolution_clock::now();
  double write_time =
      duration_cast<microseconds>(write_end - write_start).count();

  slog::entry_list* list = stream.get_stream();
  uint32_t size = list->size();
  ASSERT_EQ(10000, size);

  auto read_start = high_resolution_clock::now();
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i, list->at(i));
  }
  auto read_end = high_resolution_clock::now();
  double read_time = duration_cast<microseconds>(read_end - read_start).count();

  time_t tt = system_clock::to_time_t(system_clock::now());
  res << std::put_time(std::localtime(&tt), "%Y-%m-%d %X") << "\tfilter_fn2\t"
      << (write_time / kMaxEntries) << "\t" << read_time << std::endl;
}

#include "tokens.h"
#include "gtest/gtest.h"

extern std::string res_path_monolog;
extern std::string res_path_index;
extern std::string res_path_index2;
extern std::string res_path_logstore;

extern bool filter_fn1(uint64_t& record_id, const unsigned char* record,
                const uint16_t record_len, const slog::token_list& list) {
  return record_id % 10 == 0;
}

extern bool filter_fn2(uint64_t& record_id, const unsigned char* record,
                const uint16_t record_len, const slog::token_list& list) {
  bool filter = record_id % 10 == 0;
  record_id /= 10;
  return filter;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  assert(argc == 2);
  std::string res_path = std::string(argv[1]);
  res_path_monolog = res_path + "/monolog.txt";
  res_path_index = res_path + "/index.txt";
  res_path_index2 = res_path + "/index2.txt";
  res_path_logstore = res_path + "/logstore.txt";
  return RUN_ALL_TESTS();
}

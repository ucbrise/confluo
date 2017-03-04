#include "tokens.h"
#include "gtest/gtest.h"

extern bool filter_fn1(uint64_t& record_id, const unsigned char* record,
                const uint16_t record_len, const monolog::token_list& list) {
  return record_id % 10 == 0;
}

extern bool filter_fn2(uint64_t& record_id, const unsigned char* record,
                const uint16_t record_len, const monolog::token_list& list) {
  bool filter = record_id % 10 == 0;
  record_id /= 10;
  return filter;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#define STL_LOCKS
#include "lock_free_ngram_idx.h"
#include "gtest/gtest.h"

#ifdef HASH4
#define FMT "%04d"
#else
#ifdef HASH3
#define FMT "%03d"
#else
#define FMT "%02d"
#endif
#endif

class LockFreeNGramIdxTest : public testing::Test {
 public:
  std::string to_string(uint64_t i) {
    char buf[5];
    sprintf(buf, FMT, i);
    return std::string(buf);
  }
};

//TEST_F(LockFreeNGramIdxTest, AccessTest) {
//  LockFreeNGramIdx idx;
//  for (uint64_t i = 0; i < 1000; i++) {
//    std::string str = to_string(i);
//    idx.add_offset(str.c_str(), i);
//  }
//
//  for (uint64_t i = 0; i < 1000; i++) {
//    std::string str = to_string(i);
//    OffsetList* list = idx.get_offsets(str.c_str());
//    ASSERT_EQ(i, list->at(0));
//  }
//}

#ifndef TEST_BITMAP_TEST_H_
#define TEST_BITMAP_TEST_H_

#include "bitmap.h"
#include "bit_utils.h"

#include "gtest/gtest.h"

using namespace confluo;
using namespace utils;

class BitmapTest : public testing::Test {
 public:
  const uint64_t kBitmapSize = (1024ULL * 1024ULL);  // 1 KBits

 protected:
  virtual void SetUp() override {
    bitmap_ = new bitmap(kBitmapSize);
  }

  virtual void TearDown() override {
    delete bitmap_;
  }

  bitmap *bitmap_;
};

TEST_F(BitmapTest, GetSetBitTest) {
  for (uint64_t i = 0; i < kBitmapSize; i++) {
    if (i % 2 == 0) {
      bitmap_->set_bit(i);
    }
  }

  for (uint64_t i = 0; i < kBitmapSize; i++) {
    bool val = bitmap_->get_bit(i);
    ASSERT_EQ(i % 2 == 0, val);
  }
}

TEST_F(BitmapTest, GetSetValPosTest) {
  uint64_t pos = 0;
  for (uint64_t i = 0; i < 10000; i++) {
    bitmap_->set_val_pos(pos, i, bit_utils::bit_width(i));
    pos += bit_utils::bit_width(i);
  }

  pos = 0;
  for (uint64_t i = 0; i < 10000; i++) {
    uint64_t val = bitmap_->get_val_pos<uint64_t>(pos, bit_utils::bit_width(i));
    ASSERT_EQ(i, val);
    pos += bit_utils::bit_width(i);
  }
}

#endif /* TEST_BITMAP_TEST_H_ */

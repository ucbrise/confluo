#ifndef TEST_BITMAP_ARRAY_TEST_H_
#define TEST_BITMAP_ARRAY_TEST_H_

#include "bitmap_array.h"
#include "bit_utils.h"

#include "gtest/gtest.h"

using namespace confluo;

class BitmapArrayTest : public testing::Test {
 public:
  const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes
  const uint8_t kBitWidth = 20;  // 20 bits
};

TEST_F(BitmapArrayTest, UnsizedBitmapArrayTest) {
  unsized_bitmap_array<uint64_t> array(kArraySize, kBitWidth);
  for (uint64_t i = 0; i < kArraySize; i++) {
    array[i] = i;
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(i, array[i]);
  }
}

TEST_F(BitmapArrayTest, UnsignedBitmapArrayTest) {
  unsigned_bitmap_array<uint64_t> array(kArraySize, kBitWidth);
  for (uint64_t i = 0; i < kArraySize; i++) {
    array[i] = i;
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(i, array[i]);
  }
}

TEST_F(BitmapArrayTest, SignedBitmapArrayTest) {
  signed_bitmap_array<int64_t> array(kArraySize, kBitWidth);
  for (int64_t i = 0; i < static_cast<int64_t>(kArraySize); i++) {
    if (i % 2 == 0) {
      array[i] = i;
    } else {
      array[i] = -i;
    }
  }

  for (int64_t i = 0; i < static_cast<int64_t>(kArraySize); i++) {
    if (i % 2 == 0) {
      ASSERT_EQ(i, array[i]);
    } else {
      ASSERT_EQ(-i, array[i]);
    }
  }
}

#endif /* TEST_BITMAP_ARRAY_TEST_H_ */

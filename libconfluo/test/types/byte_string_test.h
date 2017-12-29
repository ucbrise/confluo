#ifndef CONFLUO_TEST_BYTE_STRING_TEST_H_
#define CONFLUO_TEST_BYTE_STRING_TEST_H_

#include "types/byte_string.h"
#include "gtest/gtest.h"

using namespace ::confluo;

class ByteStringTest : public testing::Test {
};

TEST_F(ByteStringTest, FromUnsignedIntegerTest) {
  uint8_t a8 = 16, b8 = 19, c8 = 16;
  uint16_t a16 = 202, b16 = 665, c16 = 202;
  uint32_t a32 = 0, b32 = 65539, c32 = 0;
  uint64_t a64 = 342342, b64 = 345345, c64 = 342342;

  ASSERT_TRUE(byte_string(a8) < byte_string(b8));
  ASSERT_TRUE(byte_string(a8) <= byte_string(b8));
  ASSERT_TRUE(byte_string(b8) > byte_string(a8));
  ASSERT_TRUE(byte_string(b8) >= byte_string(a8));
  ASSERT_TRUE(byte_string(a8) != byte_string(b8));
  ASSERT_TRUE(byte_string(a8) == byte_string(c8));

  ASSERT_TRUE(byte_string(a16) < byte_string(b16));
  ASSERT_TRUE(byte_string(a16) <= byte_string(b16));
  ASSERT_TRUE(byte_string(b16) > byte_string(a16));
  ASSERT_TRUE(byte_string(b16) >= byte_string(a16));
  ASSERT_TRUE(byte_string(a16) != byte_string(b16));
  ASSERT_TRUE(byte_string(a16) == byte_string(c16));

  ASSERT_TRUE(byte_string(a32) < byte_string(b32));
  ASSERT_TRUE(byte_string(a32) <= byte_string(b32));
  ASSERT_TRUE(byte_string(b32) > byte_string(a32));
  ASSERT_TRUE(byte_string(b32) >= byte_string(a32));
  ASSERT_TRUE(byte_string(a32) != byte_string(b32));
  ASSERT_TRUE(byte_string(a32) == byte_string(c32));

  ASSERT_TRUE(byte_string(a64) < byte_string(b64));
  ASSERT_TRUE(byte_string(a64) <= byte_string(b64));
  ASSERT_TRUE(byte_string(b64) > byte_string(a64));
  ASSERT_TRUE(byte_string(b64) >= byte_string(a64));
  ASSERT_TRUE(byte_string(a64) != byte_string(b64));
  ASSERT_TRUE(byte_string(a64) == byte_string(c64));
}

TEST_F(ByteStringTest, FromSignedIntegerTest) {
  int8_t a8 = -16, b8 = 19, c8 = -16;
  int16_t a16 = -665, b16 = -202, c16 = -665;
  int32_t a32 = -65539, b32 = 0, c32 = -65539;
  int64_t a64 = 342342, b64 = 345345, c64 = 342342;

  ASSERT_TRUE(byte_string(a8) < byte_string(b8));
  ASSERT_TRUE(byte_string(a8) <= byte_string(b8));
  ASSERT_TRUE(byte_string(b8) > byte_string(a8));
  ASSERT_TRUE(byte_string(b8) >= byte_string(a8));
  ASSERT_TRUE(byte_string(a8) != byte_string(b8));
  ASSERT_TRUE(byte_string(a8) == byte_string(c8));

  ASSERT_TRUE(byte_string(a16) < byte_string(b16));
  ASSERT_TRUE(byte_string(a16) <= byte_string(b16));
  ASSERT_TRUE(byte_string(b16) > byte_string(a16));
  ASSERT_TRUE(byte_string(b16) >= byte_string(a16));
  ASSERT_TRUE(byte_string(a16) != byte_string(b16));
  ASSERT_TRUE(byte_string(a16) == byte_string(c16));

  ASSERT_TRUE(byte_string(a32) < byte_string(b32));
  ASSERT_TRUE(byte_string(a32) <= byte_string(b32));
  ASSERT_TRUE(byte_string(b32) > byte_string(a32));
  ASSERT_TRUE(byte_string(b32) >= byte_string(a32));
  ASSERT_TRUE(byte_string(a32) != byte_string(b32));
  ASSERT_TRUE(byte_string(a32) == byte_string(c32));

  ASSERT_TRUE(byte_string(a64) < byte_string(b64));
  ASSERT_TRUE(byte_string(a64) <= byte_string(b64));
  ASSERT_TRUE(byte_string(b64) > byte_string(a64));
  ASSERT_TRUE(byte_string(b64) >= byte_string(a64));
  ASSERT_TRUE(byte_string(a64) != byte_string(b64));
  ASSERT_TRUE(byte_string(a64) == byte_string(c64));
}

TEST_F(ByteStringTest, FromStringTest) {
  std::string a8 = "abc", b8 = "def", c8 = "abc";

  ASSERT_TRUE(byte_string(a8) < byte_string(b8));
  ASSERT_TRUE(byte_string(a8) <= byte_string(b8));
  ASSERT_TRUE(byte_string(b8) > byte_string(a8));
  ASSERT_TRUE(byte_string(b8) >= byte_string(a8));
  ASSERT_TRUE(byte_string(a8) != byte_string(b8));
  ASSERT_TRUE(byte_string(a8) == byte_string(c8));
}

#endif /* CONFLUO_TEST_BYTE_STRING_TEST_H_ */

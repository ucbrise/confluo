#ifndef TEST_STRING_MAP_TEST_H_
#define TEST_STRING_MAP_TEST_H_

#include "string_map.h"
#include "gtest/gtest.h"

using namespace dialog;

class StringMapTest : public testing::Test {
};

TEST_F(StringMapTest, PutGetTest) {
  string_map<int> sm;

  sm.put("abc", 1);
  sm.put("cdef", 2);
  sm.put("cdefg", 3);
  sm.put("zzqffx", 4);
  sm.put("zzqfxxg", 5);

  int val;

  ASSERT_TRUE(sm.get("abc", val));
  ASSERT_EQ(val, 1);

  ASSERT_TRUE(sm.get("cdef", val));
  ASSERT_EQ(val, 2);

  ASSERT_TRUE(sm.get("cdefg", val));
  ASSERT_EQ(val, 3);

  ASSERT_TRUE(sm.get("zzqffx", val));
  ASSERT_EQ(val, 4);

  ASSERT_TRUE(sm.get("zzqfxxg", val));
  ASSERT_EQ(val, 5);

  ASSERT_FALSE(sm.get("asdf", val));
  ASSERT_FALSE(sm.get("abcd", val));
  ASSERT_FALSE(sm.get("zzqfxxh", val));
}

TEST_F(StringMapTest, PutRemoveGetTest) {
  string_map<int> sm;

  sm.put("abc", 1);
  sm.put("cdef", 2);
  sm.put("cdefg", 3);
  sm.put("zzqffx", 4);
  sm.put("zzqfxxg", 5);

  int val;
  ASSERT_TRUE(sm.remove("abc", val));
  ASSERT_EQ(val, 1);

  ASSERT_TRUE(sm.remove("cdef", val));
  ASSERT_EQ(val, 2);

  ASSERT_TRUE(sm.remove("cdefg", val));
  ASSERT_EQ(val, 3);

  ASSERT_TRUE(sm.remove("zzqffx", val));
  ASSERT_EQ(val, 4);

  ASSERT_TRUE(sm.remove("zzqfxxg", val));
  ASSERT_EQ(val, 5);

  ASSERT_FALSE(sm.remove("asdf", val));
  ASSERT_FALSE(sm.remove("abcd", val));
  ASSERT_FALSE(sm.remove("zzqfxxh", val));

  ASSERT_FALSE(sm.get("abc", val));
  ASSERT_FALSE(sm.get("cdef", val));
  ASSERT_FALSE(sm.get("cdefg", val));
  ASSERT_FALSE(sm.get("zzqffx", val));
  ASSERT_FALSE(sm.get("zzqfxxg", val));

}

#endif /* TEST_STRING_MAP_TEST_H_ */

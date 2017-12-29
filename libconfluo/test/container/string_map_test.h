#ifndef CONFLUO_TEST_STRING_MAP_TEST_H_
#define CONFLUO_TEST_STRING_MAP_TEST_H_

#include "container/string_map.h"

#include "gtest/gtest.h"

using namespace confluo;

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

  ASSERT_TRUE(sm.get("abc", val) != -1);
  ASSERT_EQ(val, 1);

  ASSERT_TRUE(sm.get("cdef", val) != -1);
  ASSERT_EQ(val, 2);

  ASSERT_TRUE(sm.get("cdefg", val) != -1);
  ASSERT_EQ(val, 3);

  ASSERT_TRUE(sm.get("zzqffx", val) != -1);
  ASSERT_EQ(val, 4);

  ASSERT_TRUE(sm.get("zzqfxxg", val) != -1);
  ASSERT_EQ(val, 5);

  ASSERT_TRUE(sm.get("asdf", val) == -1);
  ASSERT_TRUE(sm.get("abcd", val) == -1);
  ASSERT_TRUE(sm.get("zzqfxxh", val) == -1);
}

TEST_F(StringMapTest, PutRemoveGetTest) {
  string_map<int> sm;

  sm.put("abc", 1);
  sm.put("cdef", 2);
  sm.put("cdefg", 3);
  sm.put("zzqffx", 4);
  sm.put("zzqfxxg", 5);

  int val;
  ASSERT_TRUE(sm.remove("abc", val) != -1);
  ASSERT_EQ(val, 1);

  ASSERT_TRUE(sm.remove("cdef", val) != -1);
  ASSERT_EQ(val, 2);

  ASSERT_TRUE(sm.remove("cdefg", val) != -1);
  ASSERT_EQ(val, 3);

  ASSERT_TRUE(sm.remove("zzqffx", val) != -1);
  ASSERT_EQ(val, 4);

  ASSERT_TRUE(sm.remove("zzqfxxg", val) != -1);
  ASSERT_EQ(val, 5);

  ASSERT_TRUE(sm.remove("asdf", val) == -1);
  ASSERT_TRUE(sm.remove("abcd", val) == -1);
  ASSERT_TRUE(sm.remove("zzqfxxh", val) == -1);

  ASSERT_TRUE(sm.get("abc", val) == -1);
  ASSERT_TRUE(sm.get("cdef", val) == -1);
  ASSERT_TRUE(sm.get("cdefg", val) == -1);
  ASSERT_TRUE(sm.get("zzqffx", val) == -1);
  ASSERT_TRUE(sm.get("zzqfxxg", val) == -1);

}

#endif /* CONFLUO_TEST_STRING_MAP_TEST_H_ */

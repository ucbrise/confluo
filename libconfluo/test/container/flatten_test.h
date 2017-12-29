#ifndef CONFLUO_TEST_FLATTEN_TEST_H_
#define CONFLUO_TEST_FLATTEN_TEST_H_

#include "container/flatten.h"

#include "gtest/gtest.h"

#include "container/radix_tree.h"

using namespace ::confluo;

class FlattenTest : public testing::Test {
};

TEST_F(FlattenTest, VectorTest) {
  typedef std::vector<std::vector<int>> vector_t;

  vector_t vec = { { 1, 2, 3 }, { }, { 4, 5 }, { 6 }, { }, { }, { 7, 8, 9 } };
  flattened_container<vector_t> fvec(vec);

  int n = 1;
  for (auto it = fvec.begin(); it != fvec.end(); ++it) {
    ASSERT_EQ(n, *it);
    n++;
  }
}

TEST_F(FlattenTest, RadixTreeTest) {
  radix_index tree(sizeof(int32_t), 256);
  for (int32_t i = 0; i < 256; i++)
    tree.insert(byte_string(i * 8), i);

  auto res1 = tree.range_lookup_reflogs(byte_string(0), byte_string(16));
  radix_index::rt_result res1f(res1);
  ASSERT_EQ(static_cast<size_t>(3), res1f.count());
  uint64_t i = 0;
  for (uint64_t val : res1f) {
    ASSERT_EQ(i, val);
    i++;
  }

  auto res2 = tree.range_lookup_reflogs(byte_string(1), byte_string(15));
  radix_index::rt_result res2f(res2);
  ASSERT_EQ(static_cast<size_t>(1), res2f.count());
  i = 1;
  for (uint64_t val : res2f) {
    ASSERT_EQ(i, val);
    i++;
  }
}

#endif /* CONFLUO_TEST_FLATTEN_TEST_H_ */

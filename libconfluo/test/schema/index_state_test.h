#ifndef CONFLUO_TEST_INDEX_STATE_TEST_H_
#define CONFLUO_TEST_INDEX_STATE_TEST_H_

#include "schema/index_state.h"

#include "gtest/gtest.h"

using namespace ::confluo;

class IndexStateTest : public testing::Test {
};

TEST_F(IndexStateTest, StateTransitionTest) {
  index_state_t state;
  ASSERT_FALSE(state.is_indexed());

  bool success = state.set_indexing();
  ASSERT_TRUE(success);

  state.set_indexed(3, 0.1);
  ASSERT_TRUE(state.is_indexed());
  ASSERT_EQ(3, state.id());
  ASSERT_EQ(static_cast<double>(0.1), state.bucket_size());

  state.set_unindexed();
  ASSERT_FALSE(state.is_indexed());
}

#endif /* CONFLUO_TEST_INDEX_STATE_TEST_H_ */

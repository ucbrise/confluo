#ifndef CONFLUO_TEST_FLATTEN_TEST_H_
#define CONFLUO_TEST_FLATTEN_TEST_H_

#include "container/flatten.h"

#include "gtest/gtest.h"

#include "container/cursor/batched_cursor.h"

using namespace ::confluo;

class BatchedCursorTest : public testing::Test {
 public:
  class int_cursor : public batched_cursor<int> {
   public:
    int_cursor(const std::vector<int> &elems, size_t batch_size = 64)
        : batched_cursor<int>(batch_size),
          cur_(elems.begin()),
          end_(elems.end()) {
      init();
    }

    virtual size_t load_next_batch() override {
      size_t i = 0;
      for (; i < current_batch_.size() && cur_ != end_; ++i, ++cur_) {
        current_batch_[i] = *cur_;
      }
      return i;
    }

   private:
    std::vector<int>::const_iterator cur_;
    std::vector<int>::const_iterator end_;
  };

  std::vector<int> fill(int from, int to) {
    std::vector<int> ret;
    size_t pos = 0;
    for (int i = from; i <= to; ++i, ++pos) {
      ret.push_back(i);
    }
    return ret;
  }

  void test(int from, int to) {
    auto vec = fill(from, to);
    int_cursor c(vec);
    int i = from;
    while (c.has_more()) {
      ASSERT_EQ(i, *c);
      ++i;
      c.advance();
    }
    ASSERT_EQ(to, i - 1);
  }
};

TEST_F(BatchedCursorTest, IntVectorTest) {
  test(1, 64);
  test(1, 128);
  test(1, 50);
  test(1, 100);
  test(1, 200);
}

#endif /* CONFLUO_TEST_FLATTEN_TEST_H_ */

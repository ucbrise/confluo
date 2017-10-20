#ifndef TEST_LAZY_STREAM_TEST_H_
#define TEST_LAZY_STREAM_TEST_H_

#include "lazy/lazy_stream.h"
#include "gtest/gtest.h"

using namespace ::dialog::lazy;

class LazyStreamTest : public testing::Test {
 public:
  static lazy_stream<int> ints_from(int n) {
    return lazy_stream<int>([n]() {
      return cell<int>(n, ints_from(n + 1));
    });
  }

  static lazy_stream<int> ints(int n, int m) {
    if (n > m) {
      return lazy_stream<int>();
    }
    return lazy_stream<int>([n, m]() {
      return cell<int>(n, ints(n + 1, m));
    });
  }

  static std::vector<int> vector_ints(int n, int m) {
    std::vector<int> vec;
    for (int i = n; i <= m; ++i) {
      vec.push_back(i);
    }
    return vec;
  }
};

TEST_F(LazyStreamTest, BasicTest) {
  auto first_10 = ints(1, 10);
  int i = 1;
  while (!first_10.empty()) {
    ASSERT_EQ(i, first_10.get());
    ++i;
    first_10 = first_10.pop_front();
  }
  ASSERT_EQ(11, i);
}

TEST_F(LazyStreamTest, TakeTest) {
  auto s = ints_from(1);
  auto first_10 = s.take(10);
  int i = 1;
  while (!first_10.empty()) {
    ASSERT_EQ(i, first_10.get());
    ++i;
    first_10 = first_10.pop_front();
  }
  ASSERT_EQ(11, i);
}

TEST_F(LazyStreamTest, ForEachTest) {
  auto first_10 = ints(1, 10);
  int i = 1;
  first_10.for_each([&i](int n) {
    ASSERT_EQ(i, n);
    ++i;
  });
  ASSERT_EQ(11, i);
}

TEST_F(LazyStreamTest, MapTest) {
  {
    auto first_10 = ints(1, 10);
    int i = 1;
    auto strings = first_10.map([](int i) {
      return std::to_string(i);
    });
    while (!strings.empty()) {
      ASSERT_TRUE(std::to_string(i) == strings.get());
      ++i;
      strings = strings.pop_front();
    }
    ASSERT_EQ(11, i);
  }

  {
    auto first_10 = ints(1, 10);
    int i = 1;
    auto multiple = first_10.map([](int i) {
      return i * 10;
    });
    while (!multiple.empty()) {
      ASSERT_EQ(i * 10, multiple.get());
      ++i;
      multiple = multiple.pop_front();
    }
    ASSERT_EQ(11, i);
  }
}

TEST_F(LazyStreamTest, ConcatTest) {
  auto first_10 = ints(1, 10);
  auto next_10 = ints(11, 20);
  auto first_20 = first_10 + next_10;
  int i = 1;
  first_20.for_each([&i](int n) {
    ASSERT_EQ(i, n);
    ++i;
  });
  ASSERT_EQ(21, i);
}

TEST_F(LazyStreamTest, FilterTest) {
  auto first_10 = ints(1, 10);
  auto divisible = first_10.filter([](int i) {
    return i % 2 == 0;
  });
  int i = 1;
  while (!divisible.empty()) {
    ASSERT_EQ(i * 2, divisible.get());
    ++i;
    divisible = divisible.pop_front();
  }
  ASSERT_EQ(6, i);
}

TEST_F(LazyStreamTest, FlatMapTest) {
  auto first_10 = ints(1, 10);
  auto flattened = first_10.flat_map([](int i) {
    return ints(1, i);
  });
  for (int i = 1; i <= 10; i++) {
    for (int j = 1; j <= i; j++) {
      ASSERT_FALSE(flattened.empty());
      ASSERT_EQ(j, flattened.get());
      flattened = flattened.pop_front();
    }
  }
}

TEST_F(LazyStreamTest, ContainerTest) {
  auto container = vector_ints(1, 10);
  auto first_10 = from_container(container);
  int i = 1;
  while (!first_10.empty()) {
    ASSERT_EQ(i, first_10.get());
    ++i;
    first_10 = first_10.pop_front();
  }
  ASSERT_EQ(11, i);
}

#endif /* TEST_LAZY_STREAM_TEST_H_ */

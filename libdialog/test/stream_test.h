#ifndef TEST_STREAM_TEST_H_
#define TEST_STREAM_TEST_H_

#include "lazy/stream.h"
#include "gtest/gtest.h"

using namespace ::dialog::lazy;

static stream<int> integers_from(int n) {
  return stream<int>(n, [n]() -> stream<int> {
    return integers_from(n + 1);
  });
}

static stream<int> integers(int n, int m) {
  if (n > m) {
    return stream<int>();
  }
  return stream<int>(n, [n, m]() -> stream<int> {
    return integers(n + 1, m);
  });
}

static std::vector<int> vector_of_integers(int n, int m) {
  std::vector<int> vec;
  for (int i = n; i <= m; ++i) {
    vec.push_back(i);
  }
  return vec;
}

class StreamTest : public testing::Test {
};

TEST_F(StreamTest, BasicTest) {
  auto first_10 = integers(1, 10);
  int i = 1;
  auto v = first_10.to_vector();
  while (!first_10.empty()) {
    ASSERT_EQ(i, first_10.head());
    ++i;
    first_10 = first_10.tail();
  }
  ASSERT_EQ(11, i);
}

TEST_F(StreamTest, TakeTest) {
  auto s = integers_from(1);
  auto first_10 = s.take(10);
  int i = 1;
  while (!first_10.empty()) {
    ASSERT_EQ(i, first_10.head());
    ++i;
    first_10 = first_10.tail();
  }
  ASSERT_EQ(11, i);
}

TEST_F(StreamTest, ForEachTest) {
  auto first_10 = integers(1, 10);
  int i = 1;
  first_10.for_each([&i](int n) {
    ASSERT_EQ(i, n);
    ++i;
  });
  ASSERT_EQ(11, i);
}

TEST_F(StreamTest, MapTest) {
  {
    auto first_10 = integers(1, 10);
    int i = 1;
    auto strings = first_10.map([](int i) -> std::string {
      return std::to_string(i);
    });
    while (!strings.empty()) {
      ASSERT_TRUE(std::to_string(i) == strings.head());
      ++i;
      strings = strings.tail();
    }
    ASSERT_EQ(11, i);
  }

  {
    auto first_10 = integers(1, 10);
    int i = 1;
    auto multiple = first_10.map([](int i) {
      return i * 10;
    });
    while (!multiple.empty()) {
      ASSERT_EQ(i * 10, multiple.head());
      ++i;
      multiple = multiple.tail();
    }
    ASSERT_EQ(11, i);
  }
}

TEST_F(StreamTest, ConcatTest) {
  auto first_10 = integers(1, 10);
  auto next_10 = integers(11, 20);
  auto first_20 = first_10.concat(next_10);
  int i = 1;
  first_20.for_each([&i](int n) {
    ASSERT_EQ(i, n);
    ++i;
  });
  ASSERT_EQ(21, i);
}

TEST_F(StreamTest, FilterTest) {
  auto first_10 = integers(1, 10);
  auto divisible = first_10.filter([](int i) {
    return i % 2 == 0;
  });
  int i = 1;
  while (!divisible.empty()) {
    ASSERT_EQ(i * 2, divisible.head());
    ++i;
    divisible = divisible.tail();
  }
  ASSERT_EQ(6, i);
}

TEST_F(StreamTest, FlatMapTest) {
  auto first_10 = integers(1, 10);
  auto flattened = first_10.flat_map([](int i) {
    return integers(1, i);
  });
  for (int i = 1; i <= 10; i++) {
    for (int j = 1; j <= i; j++) {
      ASSERT_FALSE(flattened.empty());
      ASSERT_EQ(j, flattened.head());
      flattened = flattened.tail();
    }
  }
}

TEST_F(StreamTest, DistinctTest) {
  auto first_10 = integers(1, 10);
  auto flattened = first_10.flat_map([](int i) {
    return integers(1, i);
  });
  auto distinct = flattened.distinct();
  int i = 1;
  while (!distinct.empty()) {
    ASSERT_EQ(i, distinct.head());
    ++i;
    distinct = distinct.tail();
  }
  ASSERT_EQ(11, i);
}

TEST_F(StreamTest, ContainerTest) {
  auto container = vector_of_integers(1, 10);
  auto first_10 = container_to_stream(container);
  int i = 1;
  while (!first_10.empty()) {
    ASSERT_EQ(i, first_10.head());
    ++i;
    first_10 = first_10.tail();
  }
  ASSERT_EQ(11, i);
}

#endif /* TEST_STREAM_TEST_H_ */

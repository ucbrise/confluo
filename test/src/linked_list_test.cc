#define COLLECT_LIST_STATS

#include "linked_list.h"

#include "gtest/gtest.h"

class LinkedListTest : public testing::Test {
 public:
  const uint64_t kListSize = (1024ULL * 1024ULL);  // 1 KBytes
};

TEST_F(LinkedListTest, SequentialAccessTest) {
  fprintf(stderr, "Creating linked list...\n");
  LinkedList array;

  fprintf(stderr, "Inserting elements...\n");
  for (uint32_t i = 0; i < kListSize; i++) {
    array.push_back(i);
  }
  ASSERT_EQ(array.size(), kListSize);

  PRINT_STATS;
  FLUSH_STATS;

  uint32_t i = 0;
  fprintf(stderr, "Checking for correctness...\n");
  for (auto val : array) {
    ASSERT_EQ(val, i);
    i++;
  }
  ASSERT_EQ(i, kListSize);

  PRINT_STATS;
}

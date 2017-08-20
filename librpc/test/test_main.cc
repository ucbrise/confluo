#define GTEST_HAS_TR1_TUPLE 0

#include "gtest/gtest.h"

#include "server_client_test.h"
#include "reader_test.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

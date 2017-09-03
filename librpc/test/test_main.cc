
#define GTEST_HAS_TR1_TUPLE 0

#include "gtest/gtest.h"

#include "client_connection_test.h"
#include "client_readops_test.h"
#include "client_writeops_test.h"
#include "replication_test.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

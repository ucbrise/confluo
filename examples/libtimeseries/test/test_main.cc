#define GTEST_HAS_TR1_TUPLE 0

#include "error_handling.h"
#include "gtest/gtest.h"
#include "timeseries_db_test.h"

int main(int argc, char** argv) {
  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

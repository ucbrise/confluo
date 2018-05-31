#define GTEST_HAS_TR1_TUPLE 0

#include "gtest/gtest.h"
#include "client_connection_test.h"
#include "client_read_ops_test.h"
#include "client_write_ops_test.h"
#include "error_handling.h"

int main(int argc, char **argv) {
  utils::error_handling::install_signal_handler(argv[0], SIGSEGV, SIGKILL,
                                                SIGSTOP);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

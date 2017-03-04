#include "gtest/gtest.h"

extern std::string res_path_monolog;

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  assert(argc == 2);
  std::string res_path = std::string(argv[1]);
  res_path_monolog = res_path + "/monolog";
  return RUN_ALL_TESTS();
}

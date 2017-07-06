#include "gtest/gtest.h"

#include "dialog_table_test.h"
#include "expression_compiler_test.h"
#include "expressions_test.h"
#include "filter_test.h"
#include "monolog_test.h"
#include "tiered_index_test.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

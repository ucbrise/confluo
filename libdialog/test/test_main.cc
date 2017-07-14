#include "gtest/gtest.h"

#include "aggregate_test.h"
#include "byte_string_test.h"
#include "column_test.h"
#include "compiled_predicate_test.h"
#include "compiled_expression_test.h"
#include "data_types_test.h"
#include "dialog_table_test.h"
#include "expression_compiler_test.h"
#include "expression_test.h"
#include "filter_test.h"
#include "immutable_value_test.h"
#include "index_state_test.h"
#include "minterm_test.h"
#include "monolog_test.h"
#include "mutable_value_test.h"
#include "radix_tree_test.h"
#include "schema_test.h"
#include "table_metadata_test.h"
#include "tiered_index_test.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

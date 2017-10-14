
#define GTEST_HAS_TR1_TUPLE 0

#include "gtest/gtest.h"

#include "aggregate_test.h"
#include "aggregated_reflog_test.h"
#include "bitmap_test.h"
#include "bitmap_array_test.h"
#include "byte_string_test.h"
#include "column_test.h"
#include "data_types_test.h"
#include "delta_encoded_array_test.h"
#include "dialog_table_test.h"
#include "expression_compiler_test.h"
#include "expression_parser_test.h"
#include "filter_test.h"
#include "flatten_test.h"
#include "immutable_value_test.h"
#include "index_state_test.h"
#include "mempool_test.h"
#include "mempool_stat_test.h"
#include "monolog_test.h"
#include "mutable_value_test.h"
#include "periodic_task_test.h"
#include "radix_tree_test.h"
#include "record_batch_test.h"
#include "schema_parser_test.h"
#include "schema_test.h"
#include "string_map_test.h"
#include "table_metadata_test.h"
#include "task_test.h"
#include "thread_manager_test.h"
#include "tiered_index_test.h"
#include "trigger_compiler_test.h"
#include "trigger_parser_test.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

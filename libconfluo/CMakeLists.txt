set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(CONFLUO_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/confluo)
set(UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libutils/utils)

include_directories(${GTEST_INCLUDE_DIR}
  ${CONFLUO_INCLUDE}
  ${UTILS_INCLUDE}
  ${Boost_INCLUDE_DIRS})

if (BUILD_TESTS)
  # Build test
  add_executable(ctest
          confluo/read_tail.h
          confluo/atomic_multilog.h
          confluo/types
          confluo/types/type_properties.h
          confluo/types/numeric.h
          confluo/types/mutable_value.h
          confluo/types/raw_data.h
          confluo/types/key_ops.h
          confluo/types/immutable_value.h
          confluo/types/string_ops.h
          confluo/types/data_type.h
          confluo/types/serde_ops.h
          confluo/types/byte_string.h
          confluo/types/arithmetic_ops.h
          confluo/types/primitive_types.h
          confluo/types/relational_ops.h
          confluo/types/type_manager.h
          confluo/filter_log.h
          confluo/trigger.h
          confluo/atomic_multilog_metadata.h
          confluo/planner
          confluo/planner/query_ops.h
          confluo/planner/query_plan.h
          confluo/planner/query_planner.h
          confluo/aggregate
          confluo/aggregate/aggregate.h
          confluo/aggregate/aggregate_manager.h
          confluo/aggregate/aggregate_info.h
          confluo/aggregate/aggregate_ops.h
          confluo/exceptions.h
          confluo/parser
          confluo/parser/expression_compiler.h
          confluo/parser/trigger_parser.h
          confluo/parser/aggregate_parser.h
          confluo/parser/expression_parser.h
          confluo/parser/schema_parser.h
          confluo/storage
          confluo/storage/encoded_ptr.h
          confluo/storage/swappable_encoded_ptr.h
          confluo/storage/memory_stat.h
          confluo/storage/swappable_ptr.h
          confluo/storage/ptr_metadata.h
          confluo/storage/reference_counts.h
          confluo/storage/allocator.h
          confluo/storage/storage.h
          confluo/storage/ptr_aux_block.h
          confluo/storage/storage_allocator.h
          confluo/storage/storage_utils.h
          confluo/aggregated_reflog.h
          confluo/archival
          confluo/archival/archival_actions.h
          confluo/archival/atomic_multilog_archiver.h
          confluo/archival/load_utils.h
          confluo/archival/archiver.h
          confluo/archival/archival_mode.h
          confluo/archival/io
          confluo/archival/io/incremental_file_writer.h
          confluo/archival/io/incremental_file_offset.h
          confluo/archival/io/incremental_file_stream.h
          confluo/archival/io/incremental_file_reader.h
          confluo/archival/index_log_archiver.h
          confluo/archival/filter_archiver.h
          confluo/archival/index_archiver.h
          confluo/archival/monolog_linear_archiver.h
          confluo/archival/filter_log_archiver.h
          confluo/archival/archival_metadata.h
          confluo/archival/archival_utils.h
          confluo/container
          confluo/container/flatten.h
          confluo/container/record_offset_range.h
          confluo/container/cursor
          confluo/container/cursor/offset_cursors.h
          confluo/container/cursor/record_cursors.h
          confluo/container/cursor/batched_cursor.h
          confluo/container/cursor/alert_cursor.h
          confluo/container/bitmap
          confluo/container/bitmap/bitmap_array.h
          confluo/container/bitmap/delta_encoded_array.h
          confluo/container/bitmap/bitmap.h
          confluo/container/unique_byte_array.h
          confluo/container/string_map.h
          confluo/container/reflog.h
          confluo/container/data_log.h
          confluo/container/lazy
          confluo/container/lazy/stream.h
          confluo/container/monolog
          confluo/container/monolog/monolog_exp2.h
          confluo/container/monolog/monolog_iterator.h
          confluo/container/monolog/monolog.h
          confluo/container/monolog/monolog_exp2_linear.h
          confluo/container/monolog/monolog_linear.h
          confluo/container/monolog/monolog_linear_bucket.h
          confluo/container/radix_tree.h
          confluo/schema
          confluo/schema/field.h
          confluo/schema/record.h
          confluo/schema/record_batch.h
          confluo/schema/schema.h
          confluo/schema/column.h
          confluo/schema/index_state.h
          confluo/schema/column_snapshot.h
          confluo/schema/schema_snapshot.h
          confluo/threads
          confluo/threads/thread_manager.h
          confluo/threads/task_pool.h
          confluo/threads/periodic_task.h
          confluo/index_log.h
          confluo/alert_index.h
          confluo/compression
          confluo/compression/confluo_encoder.h
          confluo/compression/delta_encoder.h
          confluo/compression/lz4_decoder.h
          confluo/compression/lz4_encoder.h
          confluo/compression/delta_decoder.h
          confluo/confluo_store.h
          confluo/trigger_log.h
          confluo/conf
          confluo/conf/configuration_params.h
          confluo/conf/defaults.h
          confluo/filter.h
          confluo/alert.h
          test/filter_test.h
          test/types
          test/types/data_types_test.h
          test/types/mutable_value_test.h
          test/types/immutable_value_test.h
          test/types/ip_address.h
          test/types/size_type.h
          test/types/type_manager_test.h
          test/types/byte_string_test.h
          test/atomic_multilog_metadata_test.h
          test/test_main.cc
          test/atomic_multilog_test.h
          test/test_utils.h
          test/aggregate
          test/aggregate/aggregate_test.h
          test/parser
          test/parser/aggregate_parser_test.h
          test/parser/expression_compiler_test.h
          test/parser/schema_parser_test.h
          test/parser/expression_parser_test.h
          test/parser/trigger_parser_test.h
          test/storage
          test/storage/ptr_test.h
          test/storage/storage_allocator_test.h
          test/storage/memory_stat_test.h
          test/archival
          test/archival/monolog_linear_load_test.h
          test/archival/index_load_test.h
          test/archival/filter_load_test.h
          test/archival/monolog_linear_archival_test.h
          test/archival/index_archival_test.h
          test/archival/filter_archival_test.h
          test/container
          test/container/cursor
          test/container/cursor/batched_cursor_test.h
          test/container/bitmap
          test/container/bitmap/bitmap_test.h
          test/container/bitmap/bitmap_array_test.h
          test/container/bitmap/delta_encoded_array_test.h
          test/container/stream_test.h
          test/container/string_map_test.h
          test/container/radix_tree_test.h
          test/container/flatten_test.h
          test/container/monolog
          test/container/monolog/monolog_test.h
          test/schema
          test/schema/record_batch_test.h
          test/schema/column_test.h
          test/schema/schema_test.h
          test/schema/index_state_test.h
          test/threads
          test/threads/task_test.h
          test/threads/periodic_task_test.h
          test/threads/thread_manager_test.h
          test/compression
          test/compression/lz4_encode_test.h
          test/compression/delta_encode_test.h
          test/aggregated_reflog_test.h
          test/confluo_store_test.h)
  target_link_libraries(ctest gtest gtest_main ${CMAKE_THREAD_LIBS_INIT} ${lz4_STATIC_LIB})
  add_dependencies(ctest googletest lz4)
  set_target_properties(ctest PROPERTIES LINKER_LANGUAGE CXX)

  # Register test
  enable_testing()
  add_test(ConfluoTest ${EXECUTABLE_OUTPUT_PATH}/ctest)
endif ()

# install
install(DIRECTORY confluo/
  DESTINATION include
  FILES_MATCHING PATTERN "*")

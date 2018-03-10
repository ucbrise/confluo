#ifndef TEST_ARCHIVAL_INCREMENTAL_FILE_WRITER_TEST_H_
#define TEST_ARCHIVAL_INCREMENTAL_FILE_WRITER_TEST_H_

#include "gtest/gtest.h"
#include "archival/io/incremental_file_writer.h"

using namespace ::confluo;
using namespace ::confluo::archival;

class IncrementalFileTest : public testing::Test {
};

/**
 * Verifies that data is written to disk correctly.
 */
TEST_F(IncrementalFileTest, CopyTest) {
  incremental_file_writer writer("/tmp", "test", 1024);
  writer = incremental_file_writer("/tmp", "test2", 1024);
  writer.append<int64_t>(1);
  writer.commit<int64_t>(1);
}

#endif /* TEST_ARCHIVAL_INCREMENTAL_FILE_WRITER_TEST_H_ */

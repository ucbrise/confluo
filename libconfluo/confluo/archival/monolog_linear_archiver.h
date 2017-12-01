#ifndef CONFLUO_ARCHIVAL_MONOLOG_LINEAR_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_MONOLOG_LINEAR_ARCHIVER_H_

#include "storage/allocator.h"
#include "encoder.h"
#include "file_utils.h"
#include "incr_file_writer.h"
#include "io_utils.h"
#include "mmap_utils.h"
#include "read_tail.h"

#include <stdio.h>
#include <unistd.h>

namespace confluo {
namespace archival {

using namespace ::utils;

template<typename T, encoding_type ENCODING, size_t MAX_BUCKETS, size_t BUCKET_SIZE, size_t BUF_SIZE>
class monolog_linear_archiver {

 public:
  /**
   * Constructor
   * @param name archiver name
   * @param path path to directory to archive in
   * @param rt read tail
   * @param log monolog to archive
   */
  monolog_linear_archiver(const std::string& name, const std::string& path, const read_tail& rt,
                          monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE>& log)
      : writer_(path, name, ".dat", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        archival_tail_(0),
        rt_(rt),
        log_(log) {
  }

  /**
   * Archive buckets from the archival tail to the bucket of the offset.
   * @param offset monolog offset
   */
  void archive(size_t offset) {
    size_t stop = std::min((size_t) rt_.get(), offset);
    archival_utils::archive_monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE, ENCODING>(
                                              log_, writer_, archival_tail_, stop);
  }

 private:
  incremental_file_writer writer_;
  size_t archival_tail_;

  read_tail rt_;
  monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE>& log_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_MONOLOG_LINEAR_ARCHIVER_H_ */

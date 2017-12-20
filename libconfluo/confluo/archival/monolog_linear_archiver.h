#ifndef CONFLUO_ARCHIVAL_MONOLOG_LINEAR_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_MONOLOG_LINEAR_ARCHIVER_H_

#include "archival_utils.h"
#include "storage/encoder.h"
#include "file_utils.h"
#include "io/incr_file_writer.h"
#include "read_tail.h"

namespace confluo {
namespace archival {

using namespace ::utils;
using namespace storage;
using namespace monolog;

template<typename T, encoding_type ENCODING, size_t MAX_BUCKETS, size_t BUCKET_SIZE, size_t BUF_SIZE>
class monolog_linear_archiver {

 public:
  typedef monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE> monolog;

  /**
   * Constructor.
   * @param path directory to archive in
   * @param log monolog to archive
   */
  monolog_linear_archiver(const std::string& path, monolog* log, bool clear = true)
      : writer_(path, "monolog_linear", ".dat", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        archival_tail_(0),
        log_(log) {
    if (clear) {
      file_utils::clear_dir(path);
    }
    file_utils::create_dir(path);
    writer_.init();
    writer_.close();
  }

  /**
   * Archive buckets from the archival tail to the bucket of the offset.
   * @param offset monolog offset
   */
  void archive(size_t offset) {
    writer_.open();
    archival_utils::archive_monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE, ENCODING>(
                                              log_, writer_, archival_tail_, offset);
    writer_.close();
  }

  size_t tail() {
    return archival_tail_;
  }

 private:
  incremental_file_writer writer_;
  size_t archival_tail_;
  monolog* log_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_MONOLOG_LINEAR_ARCHIVER_H_ */

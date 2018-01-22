#ifndef CONFLUO_ARCHIVAL_IO_INCR_FILE_STREAM_H_
#define CONFLUO_ARCHIVAL_IO_INCR_FILE_STREAM_H_

#include "incr_file_offset.h"

namespace confluo {
namespace archival {

class incremental_file_stream {

 public:
  incremental_file_stream()
      : incremental_file_stream("", "") {
  }

  incremental_file_stream(const std::string& path, const std::string& file_prefix)
      : file_num_(0),
        dir_path_(path),
        file_prefix_(file_prefix) {
  }

  std::string cur_path() {
    return dir_path_ + "/" + file_prefix_ + "_" + std::to_string(file_num_) + ".dat";
  }

  std::string transaction_log_path() {
    return dir_path_ + "/" + file_prefix_ + "_transaction_log.dat";
  }

  void truncate(incremental_file_offset incr_file_off, size_t transaction_log_off) {
    // TODO delete succeeding files as well
    file_utils::truncate_file(incr_file_off.path(), incr_file_off.offset());
    file_utils::truncate_file(transaction_log_path(), transaction_log_off);
  }

 protected:
  size_t file_num_;
  std::string dir_path_;
  std::string file_prefix_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_IO_INCR_FILE_STREAM_H_ */

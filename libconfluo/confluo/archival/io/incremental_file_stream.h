#ifndef CONFLUO_ARCHIVAL_IO_INCR_FILE_STREAM_H_
#define CONFLUO_ARCHIVAL_IO_INCR_FILE_STREAM_H_

#include "file_utils.h"
#include "incremental_file_offset.h"

namespace confluo {
namespace archival {

class incremental_file_stream {

 public:
  incremental_file_stream();

  incremental_file_stream(const std::string &path, const std::string &file_prefix);

  std::string cur_path();

  std::string transaction_log_path();

  void truncate(incremental_file_offset incr_file_off, size_t transaction_log_off);

 protected:
  size_t file_num_;
  std::string dir_path_;
  std::string file_prefix_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_IO_INCR_FILE_STREAM_H_ */

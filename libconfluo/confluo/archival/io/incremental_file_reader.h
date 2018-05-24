#ifndef CONFLUO_ARCHIVAL_INCR_FILE_READER_H_
#define CONFLUO_ARCHIVAL_INCR_FILE_READER_H_

#include <iostream>
#include <fstream>
#include "exceptions.h"
#include "file_utils.h"
#include "incremental_file_offset.h"
#include "incremental_file_stream.h"
#include "io_utils.h"

using namespace ::utils;

namespace confluo {
namespace archival {

class incremental_file_reader : public incremental_file_stream {

 public:
  incremental_file_reader(const std::string& path, const std::string& file_prefix);

  ~incremental_file_reader();

  template<typename T>
  incremental_file_offset advance(size_t len) {
    if (eof())
      open_next();
    else if (tell().offset() + len * sizeof(T)  > eof_offset())
      THROW(illegal_state_exception, "Stream processed incorrectly!");
    cur_ifs_->seekg(len * sizeof(T), std::ios::cur);
    return tell();
  }

  template<typename T>
  T read() {
    if (eof())
      open_next();
    else if (tell().offset() + sizeof(T) > eof_offset())
      THROW(illegal_state_exception, "Stream processed incorrectly!");
    return io_utils::read<T>(*cur_ifs_);
  }

  std::string read(size_t len);

  template<typename T>
  T read_action() {
    return io_utils::read<T>(transaction_log_ifs_);
  }

  incremental_file_offset tell();

  size_t tell_transaction_log() {
    return static_cast<size_t>(transaction_log_ifs_.tellg());
  }

  bool has_more();

  bool is_open();

 private:
  size_t eof_offset();

  // Note: std::ios::eof depends on last op
  bool eof();

  void open_next();

  static std::ifstream* open(const std::string& path);

  std::ifstream* cur_ifs_;
  std::ifstream transaction_log_ifs_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_INCR_FILE_READER_H_ */

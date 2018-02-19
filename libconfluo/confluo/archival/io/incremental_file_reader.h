#ifndef CONFLUO_ARCHIVAL_INCR_FILE_READER_H_
#define CONFLUO_ARCHIVAL_INCR_FILE_READER_H_

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
  incremental_file_reader(const std::string& path, const std::string& file_prefix)
      : incremental_file_stream(path, file_prefix) {
    cur_ifs_ = new std::ifstream(cur_path());
    transaction_log_ifs_.open(transaction_log_path());
  }

  ~incremental_file_reader() {
    if (cur_ifs_) {
      cur_ifs_->close();
      delete cur_ifs_;
    }
  }

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

  std::string read(size_t len) {
    if (eof())
      open_next();
    else if (tell().offset() + len > eof_offset())
      THROW(illegal_state_exception, "Stream processed incorrectly!");
    return io_utils::read(*cur_ifs_, len);
  }

  template<typename T>
  T read_action() {
    return io_utils::read<T>(transaction_log_ifs_);
  }

  incremental_file_offset tell() {
    return incremental_file_offset(cur_path(), cur_ifs_->tellg());
  }

  size_t tell_transaction_log() {
    return transaction_log_ifs_.tellg();
  }

  bool has_more() {
    size_t off = transaction_log_ifs_.tellg();
    transaction_log_ifs_.seekg(0, std::ios::end);
    size_t end_off = transaction_log_ifs_.tellg();
    transaction_log_ifs_.seekg(off);
    return off < end_off;
  }

  bool is_open() {
    return cur_ifs_ && cur_ifs_->is_open();
  }

 private:
  size_t eof_offset() {
    size_t cur_off = cur_ifs_->tellg();
    cur_ifs_->seekg(0, std::ios::end);
    size_t end_off = cur_ifs_->tellg();
    cur_ifs_->seekg(cur_off);
    return end_off;
  }

  // Note: std::ios::eof depends on last op
  bool eof() {
    size_t off = cur_ifs_->tellg();
    cur_ifs_->seekg(0, std::ios::end);
    size_t end_off = cur_ifs_->tellg();
    cur_ifs_->seekg(off);
    return off == end_off;
  }

  void open_next() {
    cur_ifs_->close();
    delete cur_ifs_;
    file_num_++;
    cur_ifs_ = open(cur_path());
  }

  static std::ifstream* open(const std::string& path) {
    return new std::ifstream(path);
  }

  std::ifstream* cur_ifs_;
  std::ifstream transaction_log_ifs_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_INCR_FILE_READER_H_ */

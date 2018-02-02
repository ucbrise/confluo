#ifndef CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_
#define CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_

#include "file_utils.h"
#include "incremental_file_offset.h"
#include "incremental_file_stream.h"
#include "io_utils.h"

namespace confluo {
namespace archival {

class incremental_file_writer : public incremental_file_stream {
 public:
  incremental_file_writer(const std::string& path, const std::string& file_prefix, size_t max_file_size)
      : incremental_file_stream(path, file_prefix) {
    max_file_size_ = max_file_size;
    init();
  }

  incremental_file_writer(const incremental_file_writer& other)
      : incremental_file_stream() {
    close();
    file_num_ = other.file_num_;
    dir_path_ = other.dir_path_;
    file_prefix_ = other.file_prefix_;
    max_file_size_ = other.max_file_size_;
    open();
  }

  ~incremental_file_writer() {
    close();
  }

  incremental_file_writer& operator=(const incremental_file_writer& other) {
    close();
    file_num_ = other.file_num_;
    dir_path_ = other.dir_path_;
    file_prefix_ = other.file_prefix_;
    max_file_size_ = other.max_file_size_;
    open();
    return *this;
  }

  /**
   * Initialize state.
   */
  void init() {
    if (file_utils::exists_file(transaction_log_path())) {
      std::ifstream transaction_log_ifs(transaction_log_path());
      while (file_utils::exists_file(cur_path())) {
        file_num_++;
      }
      file_num_--;
      open();
    } else {
      cur_ofs_.open(cur_path());
      transaction_log_ofs_.open(transaction_log_path());
    }
  }

  template<typename T>
  incremental_file_offset append(T* data, size_t len) {
    if (!fits_in_cur_file(sizeof(T) * len))
      open_new_next();
    incremental_file_offset incr_file_off = tell();
    io_utils::write<T>(cur_ofs_, data, len);
    cur_ofs_.flush();
    return incr_file_off;
  }

  template<typename T>
  incremental_file_offset append(T data) {
    if (!fits_in_cur_file(sizeof(T)))
      open_new_next();
    incremental_file_offset incr_file_off = tell();
    io_utils::write<T>(cur_ofs_, data);
    cur_ofs_.flush();
    return incr_file_off;
  }

  /**
   * TODO Need something more generic
   */
  template<typename T, typename U>
  incremental_file_offset append(T* t_data, size_t t_len, U* u_data, size_t u_len) {
    if (!fits_in_cur_file((sizeof(T) * t_len) + (sizeof(U) * u_len)))
      open_new_next();
    incremental_file_offset incr_file_off = tell();
    io_utils::write<T>(cur_ofs_, t_data, t_len);
    io_utils::write<U>(cur_ofs_, u_data, u_len);
    cur_ofs_.flush();
    return incr_file_off;
  }

  template<typename ACTION>
  void commit(ACTION action) {
    io_utils::write<ACTION>(transaction_log_ofs_, action);
    transaction_log_ofs_.flush();
  }

  incremental_file_offset tell() {
    return incremental_file_offset(cur_path(), cur_ofs_.tellp());
  }

  void flush() {
    cur_ofs_.flush();
    transaction_log_ofs_.flush();
  }

  void open() {
    // std::ios::in required to prevent truncation, caused by lack of std::ios::app
    cur_ofs_.open(cur_path(), std::ios::in | std::ios::out | std::ios::ate);
    transaction_log_ofs_.open(transaction_log_path(), std::ios::in | std::ios::out | std::ios::ate);
  }

  void close() {
    if (cur_ofs_.is_open())
      cur_ofs_.close();
    if (transaction_log_ofs_.is_open())
      transaction_log_ofs_.close();
  }

 private:

  bool fits_in_cur_file(size_t append_size) {
    size_t off = cur_ofs_.tellp();
    return off + append_size < max_file_size_;
  }

  incremental_file_offset open_new_next() {
    cur_ofs_.close();
    file_num_++;
    cur_ofs_.open(cur_path(), std::ios::out | std::ios::trunc);
    return tell();
  }

  std::ofstream cur_ofs_;
  std::ofstream transaction_log_ofs_;

  size_t max_file_size_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_ */

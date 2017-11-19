#ifndef CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_
#define CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_

#include "io_utils.h"

namespace confluo {
namespace archival {

class incremental_file_writer {
 public:
  incremental_file_writer(std::string path_prefix, std::string path_suffix, size_t max_file_size)
      : file_num_(0),
        path_prefix_(path_prefix),
        path_suffix_(path_suffix),
        max_file_size_(max_file_size) {
  }

  void init() {
    create_new_file();
  }

  template<typename T>
  size_t append(T* values, size_t len) {
    size_t off = open_for(sizeof(T) * len);
    io_utils::write<T>(cur_ofs_, values, len);
    return off;
  }

  template<typename T>
  size_t append(T value) {
    size_t off = open_for(sizeof(T));
    io_utils::write<T>(cur_ofs_, value);
    return off;
  }

  /**
   * TODO Need something more generic
   */
  template<typename T, typename U>
  size_t append(T* t_values, size_t t_len, U* u_values, size_t u_len) {
    size_t off = open_for((sizeof(T) * t_len) + (sizeof(U) * u_len));
    io_utils::write<T>(cur_ofs_, t_values, t_len);
    io_utils::write<U>(cur_ofs_, u_values, u_len);
    return off;
  }

  /**
   * TODO template this, interface for arbitrary schemas
   */
  void update_header(size_t val) {
    open_for(0);
    cur_ofs_.seekp(sizeof(size_t));
    io_utils::write<size_t>(cur_ofs_, val);
    cur_ofs_.seekp(0, std::ios::end);
  }

  std::string cur_path() {
    return path_prefix_ + std::to_string(file_num_) + path_suffix_;
  }

  void close() {
    if (cur_ofs_.is_open())
      cur_ofs_.close();
  }

 private:
  size_t open_for(size_t append_size) {
    if (!cur_ofs_.is_open()) {
      // std::ios::in required to prevent truncation, caused by lack of std::ios::app
      cur_ofs_ = std::ofstream(cur_path(), std::ios::in | std::ios::out | std::ios::ate);
    }
    else if (((size_t) cur_ofs_.tellp()) + append_size > max_file_size_) {
      create_new_file();
    }
    return cur_ofs_.tellp();
  }

  void create_new_file() {
    close();
    file_num_++;
    cur_ofs_ = std::ofstream(cur_path(), std::ios::out | std::ios::trunc);
    // TODO replace with default header, passed into constructor
    io_utils::write<size_t>(cur_ofs_, 0);
    io_utils::write<size_t>(cur_ofs_, 0);
  }

  std::ofstream cur_ofs_;
  size_t file_num_;
  std::string path_prefix_;
  std::string path_suffix_;
  size_t max_file_size_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_ */

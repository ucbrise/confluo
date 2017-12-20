#ifndef CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_
#define CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_

#include "incr_file_offset.h"
#include "file_utils.h"
#include "io_utils.h"

namespace confluo {
namespace archival {

class incremental_file_writer {
 public:
  incremental_file_writer(const std::string& path, const std::string& file_prefix,
                          const std::string& file_suffix, size_t max_file_size)
      : file_num_(0),
        dir_path_(path),
        file_prefix_(file_prefix),
        file_suffix_(file_suffix),
        max_file_size_(max_file_size) {
  }

  incremental_file_writer(const incremental_file_writer& other)
      : file_num_(0),
        dir_path_(other.dir_path_),
        file_prefix_(other.file_prefix_),
        file_suffix_(other.file_suffix_),
        max_file_size_(other.max_file_size_) {
  }

  ~incremental_file_writer() {
    close();
  }

  incremental_file_writer& operator=(const incremental_file_writer& other) {
    close();
    file_num_ = other.file_num_;
    dir_path_ = other.dir_path_;
    file_prefix_ = other.file_prefix_;
    file_suffix_ = other.file_suffix_;
    max_file_size_ = other.max_file_size_;
    open();
    return *this;
  }

  void init() {
    if (file_utils::exists_file(metadata_path())) {
      std::ifstream metadata_ifs(metadata_path());
      file_num_ = io_utils::read<size_t>(metadata_ifs);
      open();
    } else {
      cur_ofs_.open(cur_path());
      metadata_ofs_.open(metadata_path());
      update_last_file();
    }
  }

  template<typename T>
  incremental_file_offset append(T* values, size_t len) {
    if (!fits_in_cur_file(sizeof(T) * len))
      open_new_next();
    incremental_file_offset incr_file_off = tell();
    io_utils::write<T>(cur_ofs_, values, len);
    cur_ofs_.flush();
    update_last_file();
    return incr_file_off;
  }

  template<typename T>
  incremental_file_offset append(T value) {
    if (!fits_in_cur_file(sizeof(T)))
      open_new_next();
    incremental_file_offset incr_file_off = tell();
    io_utils::write<T>(cur_ofs_, value);
    cur_ofs_.flush();
    update_last_file();
    return incr_file_off;
  }

  /**
   * TODO Need something more generic
   */
  template<typename T, typename U>
  incremental_file_offset append(T* t_values, size_t t_len, U* u_values, size_t u_len) {
    if (!fits_in_cur_file((sizeof(T) * t_len) + (sizeof(U) * u_len)))
      open_new_next();
    incremental_file_offset incr_file_off = tell();
    io_utils::write<T>(cur_ofs_, t_values, t_len);
    io_utils::write<U>(cur_ofs_, u_values, u_len);
    cur_ofs_.flush();
    update_last_file();
    return incr_file_off;
  }

  template<typename T>
  void update_metadata(T metadata) {
    metadata_ofs_.seekp(METADATA_OFFSET);
    io_utils::write<T>(metadata_ofs_, metadata);
    metadata_ofs_.flush();
  }

  std::string cur_path() const {
    return dir_path_ + "/" + file_prefix_ + "_" + std::to_string(file_num_) + file_suffix_;
  }

  std::string metadata_path() const {
    return dir_path_ + "/" + file_prefix_ + "_metadata" + file_suffix_;
  }

  incremental_file_offset tell() {
    return incremental_file_offset(cur_path(), cur_ofs_.tellp());
  }

  void flush() {
    cur_ofs_.flush();
    metadata_ofs_.flush();
  }

  void open() {
    // std::ios::in required to prevent truncation, caused by lack of std::ios::app
    cur_ofs_.open(cur_path(), std::ios::in | std::ios::out | std::ios::ate);
    metadata_ofs_.open(metadata_path(), std::ios::in | std::ios::out | std::ios::ate);
  }

  void close() {
    if (cur_ofs_.is_open())
      cur_ofs_.close();
    if (metadata_ofs_.is_open())
      metadata_ofs_.close();
  }

 private:
  void update_last_file() {
    metadata_ofs_.seekp(0);
    io_utils::write<size_t>(metadata_ofs_, file_num_);
    metadata_ofs_.flush();
  }

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
  std::ofstream metadata_ofs_;

  size_t file_num_;
  std::string dir_path_;
  std::string file_prefix_;
  std::string file_suffix_;
  size_t max_file_size_;

  static const size_t METADATA_OFFSET = sizeof(size_t);

};

}
}

#endif /* CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_ */

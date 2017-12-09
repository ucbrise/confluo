#ifndef CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_
#define CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_

#include "incr_file_offset.h"
#include "file_utils.h"
#include "io_utils.h"

namespace confluo {
namespace archival {

// TODO modify to work when dir exists so archival for loaded data works
class incremental_file_writer {
 public:
  incremental_file_writer(std::string path, std::string file_prefix,
                          std::string file_suffix, size_t max_file_size)
      : file_num_(0),
        dir_path_(path),
        file_prefix_(file_prefix),
        file_suffix_(file_suffix),
        max_file_size_(max_file_size) {
    file_utils::create_dir(dir_path_);
    create_metadata_file();
    create_new_file();
  }

  template<typename T>
  incremental_file_offset append(T* values, size_t len) {
    incremental_file_offset incr_file_off = open_for(sizeof(T) * len);
    io_utils::write<T>(cur_ofs_, values, len);
    return incr_file_off;
  }

  template<typename T>
  incremental_file_offset append(T value) {
    incremental_file_offset incr_file_off = open_for(sizeof(T));
    io_utils::write<T>(cur_ofs_, value);
    return incr_file_off;
  }

  /**
   * TODO Need something more generic
   */
  template<typename T, typename U>
  incremental_file_offset append(T* t_values, size_t t_len, U* u_values, size_t u_len) {
    incremental_file_offset incr_file_off = open_for((sizeof(T) * t_len) + (sizeof(U) * u_len));
    io_utils::write<T>(cur_ofs_, t_values, t_len);
    io_utils::write<U>(cur_ofs_, u_values, u_len);
    cur_ofs_.flush();
    return incr_file_off;
  }

//  /**
//   * TODO make this more generic / add check for file boundary
//   * Write to an arbitrary offset of the set of files.
//   * @param incr_file_off incremental file offset
//   * @param value value to write
//   * @return the same offset
//   */
//  template<typename T>
//  incremental_file_offset overwrite(incremental_file_offset incr_file_off, T value) {
//    if (incr_file_off.path() == cur_path()) {
//      open_for(0);
//      cur_ofs_.seekp(incr_file_off.offset());
//      io_utils::write<T>(cur_ofs_, value);
//      cur_ofs_.seekp(0, std::ios::end);
//    } else {
//      std::ofstream out = std::ofstream(incr_file_off.path());
//      out.seekp(incr_file_off.offset());
//      io_utils::write<T>(cur_ofs_, value);
//    }
//    return incr_file_off;
//  }

  template<typename T>
  void update_header(T header) {
    header_ofs_.seekp(0);
    io_utils::write<T>(cur_ofs_, header);
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
    open_for(0);
    open_metadata_file();
    cur_ofs_.flush();
    header_ofs_.flush();
  }

  void close() {
    if (cur_ofs_.is_open())
      cur_ofs_.close();
    if (header_ofs_.is_open())
      header_ofs_.close();
  }

 private:
  incremental_file_offset open_for(size_t append_size) {
    if (!cur_ofs_.is_open()) {
      // std::ios::in required to prevent truncation, caused by lack of std::ios::app
      cur_ofs_ = std::ofstream(cur_path(), std::ios::in | std::ios::out | std::ios::ate);
    } else if (((size_t) cur_ofs_.tellp()) + append_size > max_file_size_) {
      create_new_file();
    }
    return tell();
  }

  size_t open_metadata_file() {
    if (!header_ofs_.is_open()) {
      // std::ios::in required to prevent truncation, caused by lack of std::ios::app
      header_ofs_ = std::ofstream(cur_path(), std::ios::in | std::ios::out | std::ios::ate);
    }
    return header_ofs_.tellp();
  }

  void create_new_file() {
    close();
    file_num_++;
    cur_ofs_ = std::ofstream(cur_path(), std::ios::out | std::ios::trunc);
  }

  void create_metadata_file() {
    header_ofs_ = std::ofstream(metadata_path(), std::ios::out | std::ios::trunc);
    // TODO replace with default header, passed into constructor
    io_utils::write<size_t>(header_ofs_, 0);
    io_utils::write<size_t>(header_ofs_, 0);
  }

  std::ofstream cur_ofs_;
  std::ofstream header_ofs_;

  size_t file_num_;
  std::string dir_path_;
  std::string file_prefix_;
  std::string file_suffix_;
  size_t max_file_size_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_ */

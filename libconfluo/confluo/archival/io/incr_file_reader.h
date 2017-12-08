#ifndef CONFLUO_ARCHIVAL_INCR_FILE_READER_H_
#define CONFLUO_ARCHIVAL_INCR_FILE_READER_H_

#include "exceptions.h"
#include "incr_file_offset.h"
#include "file_utils.h"
#include "io_utils.h"

namespace confluo {
namespace archival {

class incremental_file_reader {

 public:
  incremental_file_reader(std::string path, std::string file_prefix, std::string file_suffix)
     : file_num_(0),
       dir_path_(path),
       file_prefix_(file_prefix),
       file_suffix_(file_suffix) {
  }

  template<typename T>
  incremental_file_offset advance(size_t len) {
    if (cur_ifs_.eof()) {
      cur_ifs_.close();
      open_next_if_exists();
    } else if (tell().offset() + len * sizeof(T)  > eof_offset()) {
      THROW(illegal_state_exception, "Stream processed incorrectly!");
    }
    cur_ifs_.seekg(len * sizeof(T), std::ios::cur);
    return tell();
  }

  template<typename T>
  T read() {
    if (cur_ifs_.eof()) {
      cur_ifs_.close();
      open_next_if_exists();
    } else if (tell().offset() + sizeof(T) > eof_offset()) {
      THROW(illegal_state_exception, "Stream processed incorrectly!");
    }
    return utils::io_utils::read<T>(cur_ifs_);
  }

  std::string read(size_t len) {
    if (cur_ifs_.eof()) {
      cur_ifs_.close();
      open_next_if_exists();
    } else if (tell().offset() + len > eof_offset()) {
      THROW(illegal_state_exception, "Stream processed incorrectly!");
    }
    return utils::io_utils::read(cur_ifs_, len);
  }

  template<typename T>
  T read_header() {
    metadata_ifs_.seekg(0);
    T metadata = utils::io_utils::read<T>(metadata_ifs_);
    return metadata;
  }

  std::string cur_path() const {
    return dir_path_ + "/" + file_prefix_ + "_" + std::to_string(file_num_) + file_suffix_;
  }

  std::string metadata_path() const {
    return dir_path_ + "/" + file_prefix_ + "_metadata" + file_suffix_;
  }

  incremental_file_offset tell() {
    // TODO handle EOF
    return incremental_file_offset(cur_path(), cur_ifs_.tellg());
  }

  bool has_more() {
    if (!cur_ifs_.eof()) {
      return true;
    } else {
      cur_ifs_.close();
      file_num_++;
      return utils::file_utils::exists_file(cur_path());
    }
  }

  void open() {
    if (!cur_ifs_.is_open())
      cur_ifs_ = std::ifstream(cur_path());
    if (!metadata_ifs_.is_open())
      metadata_ifs_ = std::ifstream(metadata_path());
  }

  void close() {
    if (cur_ifs_.is_open())
      cur_ifs_.close();
    if (metadata_ifs_.is_open())
      metadata_ifs_.close();
  }

 private:
  size_t eof_offset() {
    size_t cur_off = cur_ifs_.tellg();
    cur_ifs_.seekg(0, std::ios::end);
    size_t end_off = cur_ifs_.tellg();
    cur_ifs_.seekg(cur_off);
    return end_off;
  }

  void open_next_if_exists() {
    cur_ifs_.close();
  }

  std::ifstream cur_ifs_;
  std::ifstream metadata_ifs_;

  size_t file_num_;
  std::string dir_path_;
  std::string file_prefix_;
  std::string file_suffix_;

};

template<>
std::string incremental_file_reader::read_header<std::string>() {
  metadata_ifs_.seekg(0);
  return utils::io_utils::read<std::string>(metadata_ifs_);
}

}
}

#endif /* CONFLUO_ARCHIVAL_INCR_FILE_READER_H_ */

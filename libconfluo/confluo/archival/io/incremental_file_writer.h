#ifndef CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_
#define CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_

#include <iostream>
#include <fstream>
#include "file_utils.h"
#include "incremental_file_offset.h"
#include "incremental_file_stream.h"
#include "io_utils.h"

using namespace ::utils;

namespace confluo {
namespace archival {

class incremental_file_writer : public incremental_file_stream {
 public:
  incremental_file_writer(const std::string& path, const std::string& file_prefix, size_t max_file_size);

  incremental_file_writer(const incremental_file_writer& other);

  ~incremental_file_writer();

  incremental_file_writer& operator=(const incremental_file_writer& other);

  /**
   * Initialize state.
   */
  void init();

  template<typename T>
  incremental_file_offset append(T* data, size_t len) {
    if (!fits_in_cur_file(sizeof(T) * len))
      open_new_next();
    incremental_file_offset incr_file_off = tell();
    io_utils::write<T>(*cur_ofs_, data, len);
    cur_ofs_->flush();
    return incr_file_off;
  }

  template<typename T>
  incremental_file_offset append(T data) {
    if (!fits_in_cur_file(sizeof(T)))
      open_new_next();
    incremental_file_offset incr_file_off = tell();
    io_utils::write<T>(*cur_ofs_, data);
    cur_ofs_->flush();
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
    io_utils::write<T>(*cur_ofs_, t_data, t_len);
    io_utils::write<U>(*cur_ofs_, u_data, u_len);
    cur_ofs_->flush();
    return incr_file_off;
  }

  template<typename ACTION>
  void commit(ACTION action) {
    io_utils::write<ACTION>(*transaction_log_ofs_, action);
    transaction_log_ofs_->flush();
  }

  incremental_file_offset tell();

  void flush();

  void open();

  void close();

 private:

  bool fits_in_cur_file(size_t append_size);

  incremental_file_offset open_new_next();

  static std::ofstream* open_new(const std::string& path);

   static std::ofstream* open_existing(const std::string& path);

   static void close(std::ofstream*& ofs);

  std::ofstream* cur_ofs_;
  std::ofstream* transaction_log_ofs_;

  size_t max_file_size_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_INCR_FILE_WRITER_H_ */

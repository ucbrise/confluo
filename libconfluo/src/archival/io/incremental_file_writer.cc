#include "archival/io/incremental_file_writer.h"

namespace confluo {
namespace archival {

incremental_file_writer::incremental_file_writer(const std::string &path,
                                                 const std::string &file_prefix,
                                                 size_t max_file_size)
    : incremental_file_stream(path, file_prefix) {
  max_file_size_ = max_file_size;
  init();
}

incremental_file_writer::incremental_file_writer(const incremental_file_writer &other)
    : incremental_file_stream() {
  close();
  file_num_ = other.file_num_;
  dir_path_ = other.dir_path_;
  file_prefix_ = other.file_prefix_;
  max_file_size_ = other.max_file_size_;
  cur_ofs_ = open_new(cur_path());
  transaction_log_ofs_ = open_new(transaction_log_path());
}

incremental_file_writer::~incremental_file_writer() {
  close();
}

incremental_file_writer &incremental_file_writer::operator=(const incremental_file_writer &other) {
  close();
  file_num_ = other.file_num_;
  dir_path_ = other.dir_path_;
  file_prefix_ = other.file_prefix_;
  max_file_size_ = other.max_file_size_;
  cur_ofs_ = open_new(cur_path());
  transaction_log_ofs_ = open_new(transaction_log_path());
  return *this;
}

void incremental_file_writer::init() {
  if (file_utils::exists_file(transaction_log_path())) {
    std::ifstream transaction_log_ifs(transaction_log_path());
    while (file_utils::exists_file(cur_path())) {
      file_num_++;
    }
    file_num_--;
    open();
  } else {
    cur_ofs_ = open_new(cur_path());
    transaction_log_ofs_ = open_new(transaction_log_path());
  }
}

incremental_file_offset incremental_file_writer::tell() {
  return incremental_file_offset(cur_path(), static_cast<size_t>(cur_ofs_->tellp()));
}

void incremental_file_writer::flush() {
  cur_ofs_->flush();
  transaction_log_ofs_->flush();
}

void incremental_file_writer::open() {
  cur_ofs_ = open_existing(cur_path());
  transaction_log_ofs_ = open_existing(transaction_log_path());
}

void incremental_file_writer::close() {
  if (cur_ofs_) {
    close(cur_ofs_);
  }
  if (transaction_log_ofs_)
    close(transaction_log_ofs_);
}

bool incremental_file_writer::fits_in_cur_file(size_t append_size) {
  size_t off = static_cast<size_t>(cur_ofs_->tellp());
  return off + append_size < max_file_size_;
}

incremental_file_offset incremental_file_writer::open_new_next() {
  close(cur_ofs_);
  file_num_++;
  cur_ofs_ = open_new(cur_path());
  return tell();
}

std::ofstream *incremental_file_writer::open_new(const std::string &path) {
  return new std::ofstream(path, std::ios::out | std::ios::trunc);
}

std::ofstream *incremental_file_writer::open_existing(const std::string &path) {
  // std::ios::in required to prevent truncation, caused by lack of std::ios::app
  return new std::ofstream(path, std::ios::in | std::ios::out | std::ios::ate);
}

void incremental_file_writer::close(std::ofstream *&ofs) {
  if (ofs->is_open())
    ofs->close();
  delete ofs;
  ofs = nullptr;
}

}
}
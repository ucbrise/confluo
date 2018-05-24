#include "archival/io/incremental_file_reader.h"

namespace confluo {
namespace archival {

incremental_file_reader::incremental_file_reader(const std::string &path, const std::string &file_prefix)
    : incremental_file_stream(path, file_prefix) {
  cur_ifs_ = new std::ifstream(cur_path());
  transaction_log_ifs_.open(transaction_log_path());
}

incremental_file_reader::~incremental_file_reader() {
  if (cur_ifs_) {
    cur_ifs_->close();
    delete cur_ifs_;
  }
}

std::string incremental_file_reader::read(size_t len) {
  if (eof())
    open_next();
  else if (tell().offset() + len > eof_offset())
    THROW(illegal_state_exception, "Stream processed incorrectly!");
  return io_utils::read(*cur_ifs_, len);
}

incremental_file_offset incremental_file_reader::tell() {
  return incremental_file_offset(cur_path(), static_cast<size_t>(cur_ifs_->tellg()));
}

bool incremental_file_reader::has_more() {
  size_t off = static_cast<size_t>(transaction_log_ifs_.tellg());
  transaction_log_ifs_.seekg(0, std::ios::end);
  size_t end_off = static_cast<size_t>(transaction_log_ifs_.tellg());
  transaction_log_ifs_.seekg(off);
  return off < end_off;
}

bool incremental_file_reader::is_open() {
  return cur_ifs_ && cur_ifs_->is_open();
}

size_t incremental_file_reader::eof_offset() {
  size_t cur_off = static_cast<size_t>(cur_ifs_->tellg());
  cur_ifs_->seekg(0, std::ios::end);
  size_t end_off = static_cast<size_t>(cur_ifs_->tellg());
  cur_ifs_->seekg(cur_off);
  return end_off;
}

bool incremental_file_reader::eof() {
  size_t off = static_cast<size_t>(cur_ifs_->tellg());
  cur_ifs_->seekg(0, std::ios::end);
  size_t end_off = static_cast<size_t>(cur_ifs_->tellg());
  cur_ifs_->seekg(off);
  return off == end_off;
}

void incremental_file_reader::open_next() {
  cur_ifs_->close();
  delete cur_ifs_;
  file_num_++;
  cur_ifs_ = open(cur_path());
}

std::ifstream *incremental_file_reader::open(const std::string &path) {
  return new std::ifstream(path);
}

}
}
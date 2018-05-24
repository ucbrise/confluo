#include "archival/io/incremental_file_stream.h"

namespace confluo {
namespace archival {

incremental_file_stream::incremental_file_stream()
    : incremental_file_stream("", "") {
}

incremental_file_stream::incremental_file_stream(const std::string &path, const std::string &file_prefix)
    : file_num_(0),
      dir_path_(path),
      file_prefix_(file_prefix) {
}

std::string incremental_file_stream::cur_path() {
  return dir_path_ + "/" + file_prefix_ + "_" + std::to_string(file_num_) + ".dat";
}

std::string incremental_file_stream::transaction_log_path() {
  return dir_path_ + "/" + file_prefix_ + "_transaction_log.dat";
}

void incremental_file_stream::truncate(incremental_file_offset incr_file_off, size_t transaction_log_off) {
  // TODO delete succeeding files as well
  utils::file_utils::truncate_file(incr_file_off.path(), incr_file_off.offset());
  utils::file_utils::truncate_file(transaction_log_path(), transaction_log_off);
}

}
}
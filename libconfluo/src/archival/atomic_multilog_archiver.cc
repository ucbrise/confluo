#include "archival/atomic_multilog_archiver.h"

namespace confluo {
namespace archival {

atomic_multilog_archiver::atomic_multilog_archiver()
    : atomic_multilog_archiver("", read_tail(), nullptr, nullptr, nullptr, nullptr, false) {
}

atomic_multilog_archiver::atomic_multilog_archiver(const std::string &path,
                                                   read_tail rt,
                                                   data_log *log,
                                                   filter_log *filters,
                                                   index_log *indexes,
                                                   schema_t *schema,
                                                   bool clear)
    : path_(path),
      rt_(rt),
      record_size_(schema->record_size()) {
  if (clear) {
    file_utils::clear_dir(data_log_path());
    file_utils::clear_dir(filter_log_path());
    file_utils::clear_dir(index_log_path());
  }
  data_log_archiver_ = data_log_archiver(data_log_path(), log);
  filter_log_archiver_ = filter_log_archiver(filter_log_path(), filters);
  index_log_archiver_ = index_log_archiver(index_log_path(), indexes, schema);
}

void atomic_multilog_archiver::archive(size_t offset) {
  offset = std::min(offset - offset % record_size_, (size_t) rt_.get());
  if (offset > data_log_archiver_.tail()) {
    data_log_archiver_.archive(offset);
    filter_log_archiver_.archive(offset);
    index_log_archiver_.archive(offset);
  }
}

size_t atomic_multilog_archiver::tail() {
  return data_log_archiver_.tail();
}

std::string atomic_multilog_archiver::data_log_path() {
  return path_ + "/archives/data_log/";
}

std::string atomic_multilog_archiver::filter_log_path() {
  return path_ + "/archives/filters/";
}

std::string atomic_multilog_archiver::index_log_path() {
  return path_ + "/archives/indexes/";
}

}
}
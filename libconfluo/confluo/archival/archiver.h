#ifndef CONFLUO_ARCHIVAL_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_ARCHIVER_H_

#include "container/data_log.h"
#include "monolog_linear_archiver.h"
#include "filter_log.h"
#include "filter_log_archiver.h"
#include "index_log.h"
#include "index_log_archiver.h"

namespace confluo {
namespace archival {

class archiver {
 public:
  archiver()
      : archiver("", read_tail(), nullptr, nullptr, nullptr, nullptr, false) {
  }

  archiver(const std::string& path, read_tail rt, data_log* log,
           filter_log* filters, index_log* indexes, schema_t* schema,
           bool clear = true)
      : path_(path),
        rt_(rt) {
    if (clear) {
      file_utils::clear_dir(data_log_path());
      file_utils::clear_dir(filter_log_path());
      file_utils::clear_dir(index_log_path());
    }
    data_log_archiver_ = data_log_archiver(data_log_path(), log);
    filter_log_archiver_ = filter_log_archiver(filter_log_path(), filters);
    index_log_archiver_ = index_log_archiver(index_log_path(), indexes, schema);
  }

  /**
   * Attempt to archive data log, filters and indexes
   * up to a particular data log offset. Best effort.
   * Does not allow for archival past the read tail.
   * @param offset data log offset
   */
  void archive(size_t offset) {
    offset = std::min(offset, (size_t) rt_.get());
    if (offset > data_log_archiver_.tail()) {
      data_log_archiver_.archive(offset);
      filter_log_archiver_.archive(offset);
      index_log_archiver_.archive(offset);
    }
  }

  size_t tail() {
    return data_log_archiver_.tail();
  }

  std::string data_log_path() {
    return path_ + "/archives/data_log/";
  }

  std::string filter_log_path() {
    return path_ + "/archives/filters/";
  }

  std::string index_log_path() {
    return path_ + "/archives/indexes/";
  }

 private:
  std::string path_;
  read_tail rt_;
  data_log_archiver data_log_archiver_;
  filter_log_archiver filter_log_archiver_;
  index_log_archiver index_log_archiver_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_ARCHIVER_H_ */

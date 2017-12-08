#ifndef CONFLUO_ARCHIVAL_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_ARCHIVER_H_

#include "container/data_log.h"
#include "monolog_linear_archiver.h"
#include "filter_archiver.h"
#include "index_archiver.h"

namespace confluo {
namespace archival {

class archiver {
 public:
  archiver(const std::string& path, read_tail& rt, data_log& log,
           filter_log& filters, index_log& indexes, schema_t& schema)
      : path_(path),
        rt_(rt),
        data_log_archiver_(data_log_path(), log),
        filter_log_archiver_(filter_log_path(), filters),
        index_log_archiver_(index_log_path(), indexes, schema) {
  }

  void archive(size_t data_log_offset) {
    data_log_offset = std::min(data_log_offset, (size_t) rt_.get());
    data_log_archiver_.archive(data_log_offset);
    filter_log_archiver_.archive(data_log_offset);
    index_log_archiver_.archive(data_log_offset);
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
  read_tail& rt_;
  data_log_archiver data_log_archiver_;
  filter_log_archiver<IDENTITY> filter_log_archiver_;
  index_log_archiver<IDENTITY> index_log_archiver_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_ARCHIVER_H_ */

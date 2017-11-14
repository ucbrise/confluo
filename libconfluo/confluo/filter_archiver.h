#ifndef CONFLUO_FILTER_ARCHIVER_H_
#define CONFLUO_FILTER_ARCHIVER_H_

#include "aggregated_reflog.h"
#include "archival_utils.h"
#include "encoder.h"
#include "filter.h"
#include "filter_log.h"
#include "read_tail.h"

namespace confluo {
namespace archival {

template<encoding_type ENCODING>
class filter_archiver {

 public:
  filter_archiver(const std::string& path, filter* filter)
      : filter_(filter),
        writer_(path + "/filter_data_", ".dat", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        reflog_tail_(0),
        ts_tail_(0) {
    file_utils::create_dir(path);
    writer_.init();
  }

  void archive(size_t offset) {
    //TODO iterator: auto reflogs = filter_->lookup_range_reflogs(ts_tail_, ???);
    while (true) {
      aggregated_reflog* reflog = filter_->lookup_unsafe(ts_tail_); // TODO refactor func name
      reflog_tail_ = archival_utils::archive_reflog<ENCODING>(reflog, writer_, offset, reflog_tail_);
      if (reflog_tail_ < reflog->size()) {
        break;
      }
      reflog_tail_ = 0;
      ts_tail_++;
    }
    writer_.close();
  }

 private:
  filter* filter_;
  utils::incremental_file_writer writer_;

  size_t reflog_tail_; // data in the current reflog up to this tail has been archived
  uint64_t ts_tail_; // reflogs in the filter up to this time stamp have been archived

};

template<encoding_type ENCODING>
class filter_log_archiver {

 public:
  filter_log_archiver(const std::string& name,
                  const std::string& path,
                  filter_log& filters,
                  const read_tail& rt)
      : path_(path + "/" + name + "/"),
        filter_archivers_(),
        rt_(rt),
        filters_(filters) {
    file_utils::create_dir(path_);
    add_new_archivers();
  }

  /**
   * Archive all filters up to offset.
   * @param offset data log offset
   */
  void archive(size_t offset) {
    add_new_archivers();
    for (size_t i = 0; i < filters_.size(); i++) {
      if (filters_.at(i)->is_valid()) {
        size_t max_offset = std::min(offset, (size_t) rt_.get());
        filter_archivers_.at(i)->archive(max_offset);
      }
    }
  }

 private:
  void add_new_archivers() {
    for (size_t i = filter_archivers_.size(); i < filters_.size(); i++) {
      std::string filter_path = path_ + "/filter_" + std::to_string(i) + "/";
      filter* filter = filters_.at(i);
      filter_archivers_.push_back(new filter_archiver<ENCODING>(filter_path, filter));
    }
  }

  std::string path_;
  monolog::monolog_exp2<filter_archiver<ENCODING>*> filter_archivers_;

  read_tail rt_;
  filter_log& filters_;

};

}
}

#endif /* CONFLUO_FILTER_ARCHIVER_H_ */

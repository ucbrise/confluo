#ifndef CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_

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
  filter_archiver(const std::string& path, monitor::filter* filter)
      : filter_(filter),
        reflog_writer_(path, "filter_data", ".dat", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        aggregate_writer_(path, "filter_agg", ".dat", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        refs_size_off_(),
        refs_tail_(0),
        ts_tail_(0) {
  }

  /**
   * Archive filter up to a data log offset.
   * Reflog archival format: [timestamp][reflog_size][reflog]...
   * Aggregate archival format: [timestamp][num_aggregates][aggregates]...
   * @param offset data log offset
   */
  void archive(size_t offset) {
    auto reflogs = filter_->lookup_range_reflogs(ts_tail_, (uint64_t) -1);
    for (auto it = reflogs.begin(); it != reflogs.end(); it++) {
      auto& refs = *it;
      ts_tail_ = it.key().template as<uint64_t>();
      if (refs_tail_ == 0) {
        reflog_writer_.append<uint64_t>(ts_tail_);
        refs_size_off_ = reflog_writer_.append<size_t>(0);
      }
      refs_tail_ = archival_utils::archive_reflog<ENCODING>(refs, reflog_writer_, refs_tail_, offset);
      reflog_writer_.overwrite<size_t>(refs_size_off_, refs_tail_);
      if (refs_tail_ < refs.size()) {
        break;
      }
      aggregate_writer_.append<uint64_t>(ts_tail_);
      aggregate_writer_.append<size_t>(refs.num_aggregates());
      archival_utils::archive_reflog_aggregates(refs, aggregate_writer_, refs_tail_);
      aggregate_writer_.update_metadata(ts_tail_);
      refs_tail_ = 0;
    }
    reflog_writer_.close();
    aggregate_writer_.close();
  }

 private:
  monitor::filter* filter_;
  incremental_file_writer reflog_writer_;
  incremental_file_writer aggregate_writer_;
  incremental_file_offset refs_size_off_;

  size_t refs_tail_; // data in the current reflog up to this tail has been archived
  uint64_t ts_tail_; // reflogs in the filter up to this time stamp have been archived

};

template<encoding_type ENCODING>
class filter_log_archiver {

 public:
  filter_log_archiver(const std::string& name, const std::string& path, filter_log& filters, read_tail& rt)
      : path_(path + "/" + name + "/"),
        filter_archivers_(),
        filters_(filters),
        rt_(rt) {
    file_utils::create_dir(path_);
    init_new_archivers();
  }

  ~filter_log_archiver() {
    for (auto* archiver : filter_archivers_)
      delete archiver;
  }

  /**
   * Archive all filters up to offset.
   * @param offset data log offset
   */
  void archive(size_t offset) {
    init_new_archivers();
    size_t max_offset = std::min(offset, (size_t) rt_.get());
    for (size_t i = 0; i < filters_.size(); i++) {
      if (filters_.at(i)->is_valid()) {
        filter_archivers_.at(i)->archive(max_offset);
      }
    }
  }

 private:
  /**
   * Initialize archivers for filters that haven't
   * already had archivers initialized for.
   */
  void init_new_archivers() {
    for (size_t i = filter_archivers_.size(); i < filters_.size(); i++) {
      std::string filter_path = path_ + "/filter_" + std::to_string(i) + "/";
      filter_archivers_.push_back(new filter_archiver<ENCODING>(filter_path, filters_.at(i)));
    }
  }

  std::string path_;
  monolog::monolog_exp2<filter_archiver<ENCODING>*> filter_archivers_;
  filter_log& filters_;
  read_tail& rt_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_ */

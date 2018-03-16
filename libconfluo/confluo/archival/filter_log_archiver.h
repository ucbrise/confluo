#ifndef CONFLUO_ARCHIVAL_FILTER_LOG_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_FILTER_LOG_ARCHIVER_H_

#include "archival_utils.h"
#include "storage/encoder.h"
#include "filter_archiver.h"
#include "filter_log.h"

namespace confluo {
namespace archival {

class filter_log_archiver {

 public:
  filter_log_archiver()
      : filter_log_archiver("", nullptr) {
  }

  /**
   * Constructor.
   * @param path directory to archive in
   * @param filters filter log to archive
   */
  filter_log_archiver(const std::string& path, filter_log* filters)
      : path_(path),
        filter_archivers_(),
        filters_(filters) {
  }

  ~filter_log_archiver() {
    for (auto* archiver : filter_archivers_)
      delete archiver;
  }

  /**
   * Archive all filters up to a data log offset. Create new
   * archivers for new filters since the last archive call.
   * @param offset data log offset
   */
  void archive(size_t offset) {
    init_new_archivers();
    for (size_t i = 0; i < filters_->size(); i++) {
      if (filters_->at(i)->is_valid())
        filter_archivers_.at(i)->archive(offset);
    }
  }

 private:
  /**
   * Initialize archivers for filters that haven't
   * already had archivers initialized for.
   */
  void init_new_archivers() {
    for (size_t i = filter_archivers_.size(); i < filters_->size(); i++) {
      std::string filter_path = archival_utils::filter_archival_path(path_, i);
      file_utils::create_dir(filter_path);
      filter_archivers_.push_back(new filter_archiver(filter_path, filters_->at(i)));
    }
  }

  std::string path_;
  std::vector<filter_archiver*> filter_archivers_;
  filter_log* filters_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_FILTER_LOG_ARCHIVER_H_ */

#ifndef CONFLUO_ARCHIVAL_FILTER_LOG_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_FILTER_LOG_ARCHIVER_H_

#include "archival_utils.h"
#include "filter_archiver.h"
#include "filter_log.h"

namespace confluo {
namespace archival {

class filter_log_archiver {

 public:
  filter_log_archiver();

  /**
   * Constructor.
   * @param path directory to archive in
   * @param filters filter log to archive
   */
  filter_log_archiver(const std::string &path, filter_log *filters);

  /**
   * Default destructor.
   */
  ~filter_log_archiver();

  /**
   * Archive all filters up to a data log offset. Create new
   * archivers for new filters since the last archive call.
   * @param offset data log offset
   */
  void archive(size_t offset);

 private:
  /**
   * Initialize archivers for filters that haven't
   * already had archivers initialized for.
   */
  void init_new_archivers();

  std::string path_;
  std::vector<filter_archiver *> filter_archivers_;
  filter_log *filters_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_FILTER_LOG_ARCHIVER_H_ */

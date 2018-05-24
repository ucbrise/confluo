#ifndef CONFLUO_ARCHIVAL_INDEX_LOG_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_INDEX_LOG_ARCHIVER_H_

#include "index_archiver.h"
#include "index_log.h"
#include "schema/schema.h"

namespace confluo {
namespace archival {

class index_log_archiver {

 public:
  index_log_archiver();

  /**
   * Constructor.
   * @param path directory to archive in
   * @param indexes index log to archive
   * @param schema data schema
   */
  index_log_archiver(const std::string& path, index_log* indexes, schema_t* schema);

  /**
   * Default destructor.
   */
  ~index_log_archiver();

  /**
   * Archive all indexes up to a data log offset. Create new
   * archivers for new indexes since the last archive call.
   * @param offset data log offset
   */
  void archive(size_t offset);

 private:
  /**
   * Initialize archivers for new indexes.
   */
  void init_new_archivers();

  std::string path_;
  std::vector<index_archiver*> index_archivers_;
  index_log* indexes_;
  schema_t* schema_;

};

}
}

#endif /* CONFLUO_ARCHIVAL_INDEX_LOG_ARCHIVER_H_ */

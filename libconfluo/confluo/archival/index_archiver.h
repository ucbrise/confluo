#ifndef CONFLUO_ARCHIVAL_INDEX_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_INDEX_ARCHIVER_H_

#include <unordered_map>

#include "aggregated_reflog.h"
#include "archival_actions.h"
#include "archival_utils.h"
#include "archival_metadata.h"
#include "archiver.h"
#include "schema/column.h"
#include "conf/configuration_params.h"
#include "compression/confluo_encoder.h"
#include "index_log.h"
#include "io/incremental_file_reader.h"
#include "io/incremental_file_writer.h"
#include "storage/ptr_aux_block.h"
#include "storage/ptr_metadata.h"
#include "schema/schema.h"

namespace confluo {
namespace archival {

using namespace compression;
using namespace storage;

class index_archiver : public archiver {

 public:
  /**
   * Constructor
   * @param path directory to archive in
   * @param index index to archive
   * @param column column corresponding to the index
   */
  index_archiver(const std::string &path, index::radix_index *index, const column_t column);

  /**
   * Archive index up to a data log offset.
   * @param offset data log offset
   */
  void archive(size_t offset);

 private:
  /**
   * Archive unarchived buckets of a reflog up to an offset.
   * @param key key to which reflog belongs to in radix_tree
   * @param refs reflog
   * @param offset data log offset to archive reflog up to
   */
  void archive_reflog(byte_string key, reflog &refs, size_t offset);

  /**
   * Archives a reflog bucket of a reflog corresponding to a radix tree key.
   * @param key key to which reflog belongs to in radix_tree
   * @param reflog reflog to which bucket belongs
   * @param idx reflog index at which bucket starts
   * @param bucket reflog bucket starting at idx
   * @param offset max data log offset in bucket
   * @return reflog index to which bucket is archived
   */
  size_t archive_bucket(byte_string key, reflog &refs, size_t idx, uint64_t *bucket, size_t offset);

  index::radix_index *index_;
  std::unordered_map<std::string, uint64_t> reflog_tails_;
  column_t column_;
  incremental_file_writer writer_;

};

class index_load_utils {
 public:
  /**
   * Load index archived on disk.
   * @param reader stream to load from
   * @param index index to load into
   * @return data log offset until which radix tree has been archived
   */
  static size_t load(const std::string &path, index::radix_index *index);

 private:
  /**
   * Initialize a bucket.
   * @param refs reflog
   * @param idx reflog index
   * @param encoded_bucket bucket to initialize at index
   */
  static void init_bucket_ptr(reflog *refs, size_t idx, encoded_reflog_ptr encoded_bucket);
};

}
}

#endif /* CONFLUO_ARCHIVAL_INDEX_ARCHIVER_H_ */

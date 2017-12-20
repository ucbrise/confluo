#ifndef CONFLUO_ARCHIVAL_INDEX_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_INDEX_ARCHIVER_H_

#include "storage/encoder.h"
#include "aggregated_reflog.h"
#include "archival_utils.h"
#include "schema/column.h"
#include "conf/configuration_params.h"
#include "index_log.h"
#include "io/incr_file_writer.h"
#include "read_tail.h"
#include "schema/schema.h"

namespace confluo {
namespace archival {

template<encoding_type ENCODING>
class index_archiver {

 public:
  /**
   * Constructor
   * @param path directory to archive in
   * @param index index to archive
   * @param column column corresponding to the index
   */
  index_archiver(const std::string& path, index::radix_index* index, const column_t column)
      : index_(index),
        column_(column),
        writer_(path, "index_data", ".dat", configuration_params::MAX_ARCHIVAL_FILE_SIZE) {
    writer_.init();
    writer_.close();
  }

  /**
   * Archive index up to a data log offset.
   * @param offset data log offset
   */
  void archive(size_t offset) {
    writer_.open();
    byte_string min = column_.min().to_key(column_.index_bucket_size());
    byte_string max = column_.max().to_key(column_.index_bucket_size());
    auto reflogs = index_->range_lookup_reflogs(min, max);
    for (auto it = reflogs.begin(); it != reflogs.end(); it++) {
      auto& refs = *it;
      radix_tree_archival_utils::archive_reflog<ENCODING>(it.key(), refs, writer_, 0, offset);
    }
    writer_.close();
  }

 private:
  index::radix_index* index_;
  column_t column_;
  incremental_file_writer writer_;

};

template<encoding_type ENCODING>
class index_log_archiver {

 public:
  index_log_archiver()
      : index_log_archiver("", nullptr, nullptr, false) {
  }

  /**
   * Constructor.
   * @param path directory to archive in
   * @param indexes index log to archive
   * @param schema data schema
   * @param clear clear current archives if any exist
   */
  index_log_archiver(const std::string& path, index_log* indexes, schema_t* schema, bool clear = true)
     : path_(path),
       index_archivers_(),
       indexes_(indexes),
       schema_(schema) {
    if (clear) {
      file_utils::clear_dir(path);
    }
    file_utils::create_dir(path);
 }

  ~index_log_archiver() {
    for (auto* archiver : index_archivers_)
      delete archiver;
  }

  /**
   * Archive every index in the index log up to a data log offset.
   * @param offset data log offset
   */
  void archive(size_t offset) {
    init_new_archivers();
    for (size_t i = 0; i < schema_->size(); i++) {
      auto& col = (*schema_)[i];
      if (col.is_indexed()) {
        index_archivers_.at(col.index_id())->archive(offset);
      }
    }
  }

 private:
  /**
   * Initialize archivers for new indexes.
   */
  void init_new_archivers() {
    for (size_t i = 0; i < schema_->size(); i++) {
      auto& col = (*schema_)[i];
      auto id = col.index_id();
      if (col.is_indexed() && index_archivers_[id] == nullptr) {
        std::string index_path = path_ + "/index_" + std::to_string(i) + "/";
        file_utils::create_dir(index_path);
        index_archivers_[id] = new index_archiver<ENCODING>(index_path, indexes_->at(i), col);
      }
    }
  }

  std::string path_;
  std::vector<index_archiver<ENCODING>*> index_archivers_;
  index_log* indexes_;
  schema_t* schema_;

};

}
}


#endif /* CONFLUO_ARCHIVAL_INDEX_ARCHIVER_H_ */

#ifndef LIBDIALOG_DIALOG_ARCHIVAL_INDEX_ARCHIVER_H_
#define LIBDIALOG_DIALOG_ARCHIVAL_INDEX_ARCHIVER_H_

#include "aggregated_reflog.h"
#include "archival_utils.h"
#include "column.h"
#include "configuration_params.h"
#include "encoder.h"
#include "incr_file_writer.h"
#include "index_log.h"
#include "read_tail.h"
#include "schema.h"

namespace dialog {
namespace archival {

template<encoding_type ENCODING>
class index_archiver {

 public:
  index_archiver(const std::string& path, index::radix_index* index, const column_t& column)
      : index_(index),
        column_(column),
        writer_(path + "/index_data", ".dat", configuration_params::MAX_ARCHIVAL_FILE_SIZE) {
    file_utils::create_dir(path);
    writer_.init();
  }

  void archive(size_t offset) {
    byte_string min = column_.min().to_key(column_.index_bucket_size());
    byte_string max = column_.max().to_key(column_.index_bucket_size());
    auto reflogs = index_->range_lookup_reflogs(min, max);
    for (reflog& refs : reflogs) {
      archival_utils::archive_reflog<ENCODING>(refs, writer_, offset);
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
  index_log_archiver(const std::string& name,
                     const std::string& path,
                     index_log& indexes,
                     schema_t& schema,
                     read_tail& rt)
     : path_(path + "/" + name + "/"),
       index_archivers_(),
       indexes_(indexes),
       schema_(schema),
       rt_(rt) {
   file_utils::create_dir(path_);
   init_new_archivers();
 }

  ~index_log_archiver() {
    for (auto* archiver : index_archivers_)
      delete archiver;
  }

  void archive(size_t offset) {
    init_new_archivers();
    size_t max_offset = std::min(offset, (size_t) rt_.get());
    for (size_t i = 0; i < schema_.size(); i++) {
      auto& col = schema_[i];
      if (col.is_indexed()) {
        index_archivers_.at(col.index_id())->archive(max_offset);
      }
    }
  }

 private:
  void init_new_archivers() {
    for (size_t i = 0; i < schema_.size(); i++) {
      auto& col = schema_[i];
      auto id = col.index_id();
      if (col.is_indexed() && index_archivers_[id] == nullptr) {
        std::string index_path = path_ + "/filter_" + std::to_string(i) + "/";
        index_archivers_[id] = new filter_archiver<ENCODING>(index_path, indexes_.at(i), col);
      }
    }
  }

  std::string path_;
  monolog::monolog_exp2<index_archiver<ENCODING>*> index_archivers_;
  index_log& indexes_;
  schema_t& schema_;
  read_tail& rt_;

};

}
}


#endif /* LIBDIALOG_DIALOG_ARCHIVAL_INDEX_ARCHIVER_H_ */

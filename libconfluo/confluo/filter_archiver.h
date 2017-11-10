#ifndef CONFLUO_FILTER_ARCHIVER_H_
#define CONFLUO_FILTER_ARCHIVER_H_

#include "aggregated_reflog.h"
#include "incr_file_writer.h"
#include "encoder.h"
#include "file_utils.h"
#include "filter.h"
#include "filter_log.h"
#include "io_utils.h"
#include "read_tail.h"
#include "container/string_map.h"

namespace confluo {
namespace archival {

using namespace ::utils;

template<encoding_type ENCODING>
class filter_archiver {

 public:
  typedef storage::read_only_ptr<uint64_t> bucket_ptr_t;
  typedef bucket_ptr_t::decoded_ptr decoded_ptr_t;

  filter_archiver()
      : path_(),
        filter_(nullptr),
        writer_(path_ + "/filter_data_", ".dat", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        data_log_tail_(0),
        reflog_tail_(0),
        ts_tail_(0) {
  }

  filter_archiver(const std::string& path, filter* filter)
      : path_(path),
        filter_(filter),
        writer_(path_ + "/filter_data_", ".dat", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        data_log_tail_(0),
        reflog_tail_(0),
        ts_tail_(0) {
    file_utils::create_dir(path_);
  }

  void archive(size_t offset) {
    //TODO iterator: auto reflogs = filter_->lookup_range_reflogs(ts_tail_, ???);
    while (true) {
      aggregated_reflog* reflog = filter_->lookup_unsafe(ts_tail_); // TODO refactor func name
      bool is_completely_archived = archive_reflog(reflog, offset);
      if (!is_completely_archived) {
        break;
      }
      reflog_tail_ = 0;
      ts_tail_++;
    }
    writer_.close();
  }

 private:

  // TODO create monolog bucket iterator
  /**
   * Archives a reflog belonging to a filter.
   * @param reflog aggregated reflog
   * @param offset data log offset
   * @return true if reflog is completely archived, otherwise false
   */
  bool archive_reflog(aggregated_reflog* reflog, size_t offset) {
    while (data_log_tail_ < offset) {
      bucket_ptr_t bucket_ptr;
      reflog->ptr(reflog_tail_, bucket_ptr);
      auto decoded_ptr = bucket_ptr.decode_ptr();
      uint64_t* data = decoded_ptr.get();

      uint64_t max_off_lt = *std::max_element(data, data + reflog_constants::BUCKET_SIZE);
      if (max_off_lt >= offset) {
        return false;
      }

      auto* metadata = storage::ptr_metadata::get(bucket_ptr.get().internal_ptr());

      size_t encoded_size;
      auto raw_encoded_bucket = encoder::encode<uint64_t, ENCODING>(data, encoded_size);

      size_t off = writer_.append<storage::ptr_metadata, uint8_t>(
                                           metadata, 1, raw_encoded_bucket.get(), encoded_size);
      writer_.update_header(max_off_lt);
      void* encoded_bucket = ALLOCATOR.mmap(writer_.cur_path(), off, encoded_size,
                                            storage::state_type::D_ARCHIVED);
      reflog->swap_bucket_ptr(reflog_tail_, storage::encoded_ptr<uint64_t>(encoded_bucket));

      reflog_tail_ += reflog_constants::BUCKET_SIZE;
      data_log_tail_ = max_off_lt;
    }
    return true;
  }

  std::string path_;
  filter* filter_;
  utils::incremental_file_writer writer_;

  size_t data_log_tail_; // offsets up to this tail have been archived
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

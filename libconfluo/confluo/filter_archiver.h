#ifndef CONFLUO_FILTER_ARCHIVER_H_
#define CONFLUO_FILTER_ARCHIVER_H_

#include "aggregated_reflog.h"
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

template<typename encoded_ptr>
class filter_archiver {

 public:
  typedef storage::read_only_ptr<uint64_t> bucket_ptr_t;
  typedef bucket_ptr_t::decoded_ptr decoded_ptr_t;

  filter_archiver()
      : path_(),
        filter_(nullptr),
        file_num_(0),
        data_log_tail_(0),
        reflog_tail_(0),
        ts_tail_(0) {
  }

  filter_archiver(const std::string& path, filter* filter)
      : path_(path),
        filter_(filter),
        file_num_(0),
        data_log_tail_(0),
        reflog_tail_(0),
        ts_tail_(0) {
    file_utils::create_dir(path_);
    std::ofstream archival_out = create_new_archival_file();
    archival_out.close();
  }

  void archive(size_t offset, encoder<uint64_t> encoder) {
    while (true) {
      aggregated_reflog* reflog = filter_->lookup_unsafe(ts_tail_); // TODO refactor new func names
      bool is_completely_archived = archive_reflog(reflog, offset, encoder);
      if (!is_completely_archived) {
        break;
      }
      // TODO replace with iterator
      reflog_tail_ = 0;
      ts_tail_++;
    }
  }

 private:

  // TODO clean up, put inside a monolog_exp2_linear archiver / write utility functions
  // TODO remove hard coding, add monolog bucket iterator
  /**
   * Archives a reflog belonging to a filter.
   * @param id id of filter that reflog belongs to
   * @param reflog aggregated reflog
   * @param offset data log offset
   * @return true if reflog is completely archived, otherwise false
   */
  bool archive_reflog(aggregated_reflog* reflog, size_t offset, encoder<uint64_t> encoder) {

    std::ofstream out(cur_filter_file_path(), std::ios::in | std::ios::out | std::ios::ate);
    size_t file_off = out.tellp();

    while (data_log_tail_ < offset) {
      bucket_ptr_t bucket_ptr;
      reflog->ptr(reflog_tail_, bucket_ptr);
      auto decoded_ptr = bucket_ptr.decode_ptr();

      uint64_t max_off_lt = *std::max_element(decoded_ptr.get(), decoded_ptr.get() + 1024);
      if (max_off_lt >= offset) {
        return false;
      }

      storage::ptr_metadata metadata_copy = *(storage::ptr_metadata::get(bucket_ptr.get().internal_ptr()));
      metadata_copy.state_ = storage::state_type::D_ARCHIVED;

      // TODO use a better interface than encoder, preferably there should only be one iface
      // since encoded_ptr and encoder duplicates functionality
      uint8_t* encoded_bucket = encoder.encode(decoded_ptr.get());

      size_t encoded_size = encoder.encoding_size(metadata_copy.data_size_);
      size_t md_size = sizeof(storage::ptr_metadata);
      if (file_off + md_size + encoded_size > configuration_params::MAX_ARCHIVAL_FILE_SIZE) {
        out.close();
        out = create_new_archival_file();
        file_off = out.tellp();
      }

      io_utils::write<storage::ptr_metadata>(out, metadata_copy);
      io_utils::write<uint8_t>(out, encoded_bucket, encoded_size);
      update_file_header(out, max_off_lt);

      void* data = ALLOCATOR.mmap(cur_filter_file_path(), file_off, encoded_size,
                                  storage::state_type::D_ARCHIVED);

      encoded_ptr enc_data(data);
      reflog->swap_bucket_ptr(reflog_tail_, enc_data);
      reflog_tail_ += 1024; // TODO remove hardcode
      file_off = out.tellp();
      data_log_tail_ = max_off_lt;
    }
    return true;
  }

  /**
   * Path to current file being used for archival for a filter.
   * @return path_prefix directory path
   */
  std::string cur_filter_file_path() const {
    return path_ + "/filter_data_" + std::to_string(file_num_) + ".dat";
  }

  /**
   * Create a new archival file and set its header.
   * @param offset data log offset
   * @return file stream
   */
  std::ofstream create_new_archival_file() {
    file_num_++;
    std::ofstream archival_out(cur_filter_file_path(), std::ofstream::out | std::ofstream::trunc);
    io_utils::write<size_t>(archival_out, data_log_tail_);
    io_utils::write<size_t>(archival_out, data_log_tail_);
    return archival_out;
  }

  /**
   * Update the file header with the last offset archived. Seek to EOF.
   * @param out
   * @param offset
   */
  static void update_file_header(std::ofstream& out, size_t offset) {
    // TODO create a generic file header schema
    out.seekp(sizeof(size_t));
    io_utils::write<size_t>(out, offset);
    out.seekp(0, std::ios::end);
  }

  std::string path_;
  filter* filter_;

  size_t file_num_;
  size_t data_log_tail_;
  size_t reflog_tail_;
  uint64_t ts_tail_;

};

template<typename encoded_ptr>
class filter_log_archiver {

 public:

  filter_log_archiver(const std::string& name,
                  const std::string& path,
                  filter_log& filters,
                  const read_tail& rt,
                  encoder<uint64_t> encoder)
      : path_(path + "/" + name + "/"),
        filter_archivers_(),
        rt_(rt),
        filters_(filters),
        encoder_(encoder) {
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
        filter_archivers_.at(i)->archive(max_offset, encoder_);
      }
    }
  }

 private:
  void add_new_archivers() {
    for (size_t i = filter_archivers_.size(); i < filters_.size(); i++) {
      std::string filter_path = path_ + "/filter_" + std::to_string(i) + "/";
      filter* filter = filters_.at(i);
      filter_archivers_.push_back(new filter_archiver<encoded_ptr>(filter_path, filter));
    }
  }

  std::string path_;
  monolog::monolog_exp2<filter_archiver<encoded_ptr>*> filter_archivers_;

  read_tail rt_;
  filter_log& filters_;
  encoder<uint64_t> encoder_;

};

}
}

#endif /* CONFLUO_FILTER_ARCHIVER_H_ */

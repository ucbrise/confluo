#ifndef DIALOG_FILTER_ARCHIVER_H_
#define DIALOG_FILTER_ARCHIVER_H_

#include "aggregated_reflog.h"
#include "encoder.h"
#include "file_utils.h"
#include "filter.h"
#include "filter_log.h"
#include "io_utils.h"
#include "read_tail.h"
#include "string_map.h"

namespace dialog {
namespace archival {

using namespace ::dialog::monitor;
using namespace ::utils;

class filter_archiver {

 public:
  typedef storage::read_only_ptr<uint64_t> bucket_ptr_t;
  typedef size_t filter_id_t;

  filter_archiver(const std::string& name,
                  const std::string& path,
                  const read_tail& rt,
                  filter_log& filters,
                  encoder<uint64_t> encoder)
      : path_(path + "/" + name + "/"),
        filter_file_counts_(filters.size()),
        archival_tails_ts_(filters.size()),
        archival_reflog_offsets_(filters.size()),
        archival_tail_(0),
        rt_(rt),
        filters_(filters),
        encoder_(encoder) {
    file_utils::create_dir(path_);
    for (size_t i = 0; i < filters_.size(); i++) {
      if (filters_.at(i)->is_valid()) {
        std::ofstream archival_out = create_new_archival_file(i);
        archival_out.close();
      }
    }
  }

  /**
   * Archive all filters up to offset.
   * @param offset data log offset
   */
  void archive(size_t offset) {
    for (size_t i = 0; i < filters_.size(); i++) {
      if (filters_.at(i)->is_valid()) {
        aggregated_reflog const* reflog = filters_.at(i)->lookup(archival_tails_ts_[i]);
        bool is_completely_archived = archive_reflog(i, reflog, offset);
        if (!is_completely_archived) {
          break;
        }
        archival_tails_ts_[i]++;
        archival_reflog_offsets_[i] = 0;
      }
    }
    archival_tail_ += offset;
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
  bool archive_reflog(filter_id_t id, aggregated_reflog const* reflog, size_t offset) {

    std::ofstream out(cur_filter_file_path(id), std::ios::in | std::ios::out | std::ios::ate);
    size_t file_off = out.tellp();

    while (true) { // TODO fix this condition lol
      bucket_ptr_t bucket_ptr;
      reflog->ptr(archival_reflog_offsets_[id], bucket_ptr);
      if (!is_archivable(bucket_ptr, offset)) {
        return false;
      }

      storage::ptr_metadata metadata_copy = *(storage::ptr_metadata::get(bucket_ptr.get().internal_ptr()));
      metadata_copy.state_ = storage::state_type::D_ARCHIVED;

      // TODO use a better interface than encoder, preferably there should only be one iface
      // since encoded_ptr and encoder duplicates functionality
      auto decoded_ptr = bucket_ptr.decode_ptr();
      uint8_t* encoded_bucket = encoder_.encode(decoded_ptr.get());
      // TODO clean this whole size stuff up
      size_t archived_bucket_size = encoder_.encoding_size(metadata_copy.data_size_);
      size_t md_size = sizeof(storage::ptr_metadata);

      if (file_off + md_size + archived_bucket_size > configuration_params::MAX_ARCHIVAL_FILE_SIZE) {
        out.close();
        out = create_new_archival_file(id);
        file_off = out.tellp();
      }

      io_utils::write<storage::ptr_metadata>(out, metadata_copy);
      io_utils::write<uint8_t>(out, encoded_bucket, archived_bucket_size);
      update_file_header(out, archival_tail_);

      void* data = ALLOCATOR.mmap(cur_filter_file_path(id), file_off, archived_bucket_size,
                                  storage::state_type::D_ARCHIVED);

      // TODO template encoded_ptr on the class to support arbitrary archival schemes
      storage::encoded_ptr<uint64_t> enc_data(data);
      reflog->swap_bucket_ptr(archival_reflog_offsets_[id], enc_data);
      archival_reflog_offsets_[id] += 1024; // TODO remove hardcode
      file_off = out.tellp();
    }
    return true;
  }

  /**
   * Path to current file being used for archival for a filter.
   * @param filter_id filter id
   * @return file path
   */
  std::string cur_filter_file_path(filter_id_t id) {
    return path_ + "/" + "filter_" + std::to_string(id) + "_" + std::to_string(filter_file_counts_[id]) + ".dat";
  }

  /**
   * Create a new archival file and set its header.
   * @param filter_id filter id
   * @return file stream
   */
  std::ofstream create_new_archival_file(size_t id) {
    filter_file_counts_[id]++;
    std::ofstream archival_out(cur_filter_file_path(id), std::ofstream::out | std::ofstream::trunc);
    io_utils::write<size_t>(archival_out, archival_tail_);
    io_utils::write<size_t>(archival_out, archival_tail_);
    return archival_out;
  }

  static bool is_archivable(bucket_ptr_t bucket_ptr, size_t offset) {
    // TODO account for read tail
    auto decoded_ptr = bucket_ptr.decode_ptr();
    uint64_t* ptr = decoded_ptr.get();
    for (size_t i = 0; i < 1024; i++) {
      if (ptr[i] > offset)
        return false;
    }
    return true;
  }

  /**
   * Update the file header with the last offset archived.
   * @param out
   * @param offset
   */
  static void update_file_header(std::ofstream& out, size_t offset) {
    out.seekp(sizeof(size_t));
    io_utils::write<size_t>(out, offset);
    out.seekp(0, std::ios::end);
  }

  std::string path_;
  std::vector<size_t> filter_file_counts_;

  std::vector<uint64_t> archival_tails_ts_;
  std::vector<uint64_t> archival_reflog_offsets_;
  size_t archival_tail_;

  read_tail rt_;
  filter_log& filters_;
  encoder<uint64_t> encoder_;

};

}
}

#endif /* DIALOG_FILTER_ARCHIVER_H_ */

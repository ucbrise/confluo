#ifndef CONFLUO_ARCHIVAL_MONOLOG_LINEAR_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_MONOLOG_LINEAR_ARCHIVER_H_

#include "archival_actions.h"
#include "storage/allocator.h"
#include "storage/encoder.h"
#include "file_utils.h"
#include "archival_metadata.h"
#include "io/incr_file_reader.h"
#include "io/incr_file_writer.h"
#include "read_tail.h"

namespace confluo {
namespace archival {

using namespace ::utils;
using namespace storage;
using namespace monolog;

template<typename T, encoding_type ENCODING, size_t MAX_BUCKETS, size_t BUCKET_SIZE, size_t BUF_SIZE>
class monolog_linear_archiver {

 public:
  typedef monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE> monolog;

  /**
   * Constructor.
   * @param path directory to archive in
   * @param log monolog to archive
   */
  monolog_linear_archiver(const std::string& path, monolog* log, bool clear = true)
      : writer_(path, "monolog_linear", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        archival_tail_(0),
        log_(log) {
    if (clear) {
      file_utils::clear_dir(path);
    }
    writer_.init();
    writer_.close();
  }

  /**
   * Archive buckets from the archival tail to the bucket of the offset.
   * @param offset monolog offset
   */
  void archive(size_t offset) {
    writer_.open();
    // TODO replace with bucket iterator later
    storage::read_only_encoded_ptr<T> bucket_ptr;
    while (archival_tail_ < offset) {
      log_->ptr(archival_tail_, bucket_ptr);
      T* data = bucket_ptr.get().template ptr_as<T>();
      auto* metadata = storage::ptr_metadata::get(data);
      if (metadata->state_ != state_type::D_IN_MEMORY) {
        archival_tail_ += BUCKET_SIZE;
        continue;
      }
      if (log_->size() - archival_tail_ < BUCKET_SIZE) {
        break;
      }
      archive_bucket(data);
      archival_tail_ += BUCKET_SIZE;
    }
    writer_.close();
  }

  size_t tail() {
    return archival_tail_;
  }

 private:
  /**
   * Archive bucket and swap the pointer to the in-memory
   * bucket in the monolog with the archived version.
   * @param bucket bucket to archive
   */
  void archive_bucket(T* bucket) {
    auto metadata = ptr_metadata::get(bucket);
    auto encoded_bucket = encoder::encode<T, ENCODING>(bucket);
    size_t enc_size = encoded_bucket.size();
    auto off = writer_.append<ptr_metadata, uint8_t>(metadata, 1, encoded_bucket.get(), enc_size);

    auto action = monolog_linear_archival_action(archival_tail_ + BUCKET_SIZE);
    writer_.commit<monolog_linear_archival_action>(action);

    void* archived_bucket = ALLOCATOR.mmap(off.path(), off.offset(), enc_size, state_type::D_ARCHIVED);
    log_->swap_bucket_ptr(archival_tail_ / BUCKET_SIZE, encoded_ptr<T>(archived_bucket));
  }

  incremental_file_writer writer_;
  size_t archival_tail_;
  monolog* log_;

};

class monolog_linear_load_utils {
public:

  /**
   * Load a monolog_linear archived on disk.
   * @param path path to data
   * @param log log to load into
   */
  template<typename T, size_t MAX_BUCKETS, size_t BUCKET_SIZE, size_t BUF_SIZE>
  static void load(const std::string& path, monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE>& log) {
    incremental_file_reader reader(path, "monolog_linear");
    size_t load_offset = 0;
    while (reader.has_more()) {
      auto action = reader.read_action<monolog_linear_archival_action>();
      LOG_INFO << action.archival_tail();
      incremental_file_offset off = reader.tell();
      size_t size = reader.read<ptr_metadata>().data_size_;

      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), size, state_type::D_ARCHIVED);
      log.init_bucket_ptr(load_offset / BUCKET_SIZE, encoded_ptr<T>(encoded_bucket));

      log.reserve(BUCKET_SIZE);
      reader.advance<uint8_t>(size);
      load_offset += BUCKET_SIZE;
    }
    reader.truncate(reader.tell(), reader.tell_transaction_log());
  }
};

}
}

#endif /* CONFLUO_ARCHIVAL_MONOLOG_LINEAR_ARCHIVER_H_ */

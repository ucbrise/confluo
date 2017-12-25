#ifndef CONFLUO_ARCHIVAL_MONOLOG_ARCHIVAL_UTILS_H_
#define CONFLUO_ARCHIVAL_MONOLOG_ARCHIVAL_UTILS_H_

#include "storage/encoder.h"
#include "file_utils.h"
#include "io/incr_file_offset.h"
#include "io/incr_file_reader.h"
#include "io/incr_file_writer.h"
#include "radix_tree_archival_utils.h"
#include "read_tail.h"

namespace confluo {
namespace archival {

using namespace ::utils;
using namespace storage;
using namespace monolog;

class monolog_linear_archival_utils {

 public:
  /**
   * Archive buckets of a monolog_linear until a given offset.
   * Format: [bucket][bucket][bucket]...
   * @param monolog monolog_linear to archive
   * @param writer archival output
   * @param start monolog offset to start archival from
   * @param stop monolog offset to end archival at
   * @return monolog offset archived up to
   */
  template<typename T, size_t MAX_BUCKETS, size_t BUCKET_SIZE, size_t BUFFER_SIZE, encoding_type ENCODING>
  static size_t archive(monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUFFER_SIZE>* monolog,
                        incremental_file_writer& writer, size_t start, size_t stop) {
    // TODO replace with bucket iterator later
    size_t archival_tail = start;
    storage::read_only_encoded_ptr<T> bucket_ptr;
    while (archival_tail < stop) {
      monolog->ptr(archival_tail, bucket_ptr);
      auto decoded_ptr = bucket_ptr.decode_ptr();
      auto* metadata = storage::ptr_metadata::get(bucket_ptr.get().ptr());
      T* data = decoded_ptr.get();

      if (metadata->state_ != state_type::D_IN_MEMORY) {
        archival_tail += BUCKET_SIZE;
        continue;
      }

      size_t encoded_size;
      encoder::raw_encoded_ptr raw_encoded_bucket = encoder::encode<T, ENCODING>(data, encoded_size);
      auto off = writer.append<ptr_metadata, uint8_t>(metadata, 1, raw_encoded_bucket.get(), encoded_size);

      auto archival_metadata = monolog_linear_archival_metadata(archival_tail + BUCKET_SIZE);
      writer.update_metadata<monolog_linear_archival_metadata>(archival_metadata);

      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), encoded_size, state_type::D_ARCHIVED);
      monolog->swap_bucket_ptr(archival_tail / BUCKET_SIZE, encoded_ptr<T>(encoded_bucket));
      archival_tail += BUCKET_SIZE;
    }
    return archival_tail;
  }

  /**
   * Load a monolog_linear archived on disk.
   * @param path path to data
   * @param log log to load into
   */
  template<typename T, size_t MAX_BUCKETS, size_t BUCKET_SIZE, size_t BUF_SIZE>
  static void load(const std::string& path, monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUF_SIZE>& log) {
    incremental_file_reader reader(path, "monolog_linear");
    reader.open();
    auto archival_metadata = reader.read_metadata<monolog_linear_archival_metadata>();

    size_t load_offset = 0;
    while (reader.has_more() && load_offset < archival_metadata.archival_tail()) {
      incremental_file_offset off = reader.tell();
      size_t size = reader.read<ptr_metadata>().data_size_;

      void* encoded_bucket = ALLOCATOR.mmap(off.path(), off.offset(), size, state_type::D_ARCHIVED);
      log.init_bucket_ptr(load_offset / BUCKET_SIZE, encoded_ptr<T>(encoded_bucket));

      log.reserve(BUCKET_SIZE);
      reader.advance<uint8_t>(size);
      load_offset += BUCKET_SIZE;
    }

    if (load_offset != archival_metadata.archival_tail()) {
      THROW(illegal_state_exception, "Archived data could not be loaded!");
    }
    incr_file_utils::truncate_rest(reader.tell());
  }
};

}
}

#endif /* CONFLUO_ARCHIVAL_MONOLOG_ARCHIVAL_UTILS_H_ */

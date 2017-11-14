#ifndef CONFLUO_MONOLOG_LINEAR_ARCHIVER_H_
#define CONFLUO_MONOLOG_LINEAR_ARCHIVER_H_

#include "encoder.h"
#include "file_utils.h"
#include "incr_file_writer.h"
#include "io_utils.h"
#include "mmap_utils.h"
#include "read_tail.h"
#include "string_map.h"

#include <stdio.h>
#include <unistd.h>

namespace confluo {
namespace archival {

using namespace ::utils;

template<typename T, encoding_type ENCODING, size_t MAX_BLOCKS, size_t BLOCK_SIZE, size_t BUF_SIZE>
class monolog_linear_archiver {

 public:
  typedef storage::read_only_ptr<T> block_ptr_t;

  /**
   * Constructor
   * @param name archiver name
   * @param path path to directory to archive in
   * @param rt read tail
   * @param log monolog to archive
   */
  monolog_linear_archiver(const std::string& name,
                          const std::string& path,
                          const read_tail& rt,
                          monolog_linear<T, MAX_BLOCKS, BLOCK_SIZE, BUF_SIZE>& log)
      : writer_(path + "/" + name + "/", ".dat", configuration_params::MAX_ARCHIVAL_FILE_SIZE),
        archival_tail_(0),
        rt_(rt),
        log_(log) {
    file_utils::create_dir(path + "/" + name + "/");
    writer_.init();
  }

  /**
   * Archive blocks from the archival tail to the block of the offset.
   * @param offset monolog offset
   */
  void archive(size_t offset) {
    // TODO replace with block iterator later
    size_t start = archival_tail_ / BLOCK_SIZE;
    size_t stop = std::min(offset / BLOCK_SIZE, (size_t) rt_.get() / BLOCK_SIZE); // TODO fix cast

    for (size_t i = start; i < stop; i++) {

      block_ptr_t block_ptr;
      log_.ptr(archival_tail_, block_ptr);

      auto* metadata = storage::ptr_metadata::get(block_ptr.get().internal_ptr());
      auto decoded_ptr = block_ptr.decode_ptr();

      size_t encoded_size;
      auto raw_encoded_block  = encoder::encode<T, ENCODING>(decoded_ptr.get(), encoded_size);

      size_t off = writer_.append<storage::ptr_metadata, uint8_t>(metadata, 1, raw_encoded_block.get(),
                                                                  encoded_size);
      writer_.update_header(archival_tail_);
      void* archived_data = ALLOCATOR.mmap(writer_.cur_path(), off, encoded_size,
                                           storage::state_type::D_ARCHIVED);
      log_.swap_block_ptr(i, storage::encoded_ptr<T>(archived_data));

      archival_tail_ += BLOCK_SIZE;
    }
  }

 private:

  utils::incremental_file_writer writer_;
  size_t archival_tail_;

  read_tail rt_;
  monolog_linear<T, MAX_BLOCKS, BLOCK_SIZE, BUF_SIZE>& log_;

};

}
}

#endif /* CONFLUO_MONOLOG_LINEAR_ARCHIVER_H_ */

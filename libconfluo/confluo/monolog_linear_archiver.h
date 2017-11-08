#ifndef DIALOG_MONOLOG_LINEAR_ARCHIVER_H_
#define DIALOG_MONOLOG_LINEAR_ARCHIVER_H_

#include "dialog_allocator.h"
#include "encoder.h"
#include "file_utils.h"
#include "io_utils.h"
#include "mmap_utils.h"
#include "read_tail.h"
#include "string_map.h"

#include <stdio.h>
#include <unistd.h>

namespace dialog {
namespace archival {

using namespace ::utils;

template<typename T, size_t MAX_BLOCKS, size_t BLOCK_SIZE, size_t BUF_SIZE>
class monolog_linear_archiver {

 public:

  // TODO this will be replaced with either encoder iface or something to do w/ encoded_ptr

  /**
   * Constructor
   * @param name name
   * @param path path
   * @param rt global read tail
   * @param log monolog to archive
   * @param transform transform function to apply before archiving each block
   */
  monolog_linear_archiver(const std::string& name,
                          const std::string& path,
                          const read_tail& rt,
                          monolog_linear<T, MAX_BLOCKS, BLOCK_SIZE, BUF_SIZE>& log,
                          encoder<T> encoder)
      : path_(path + "/" + name + "/"),
        cur_file_count_(0),
        archival_tail_(0),
        rt_(rt),
        log_(log),
        encoder_(encoder) {
    file_utils::create_dir(path_);
    std::ofstream archival_out = create_new_archival_file();
    archival_out.close();
  }

  /**
   * Archive blocks from the archival tail to the block of the offset.
   * @param offset monolog offset
   */
  void archive(size_t offset) {
    size_t start = archival_tail_ / BLOCK_SIZE;
    size_t stop = std::min(offset / BLOCK_SIZE, (size_t) rt_.get() / BLOCK_SIZE); // TODO fix cast

    // std::ios::in required to prevent truncation, caused by lack of std::ios::app
    std::ofstream archival_out(cur_file_path(), std::ios::in | std::ios::out | std::ios::ate);
    size_t file_off = archival_out.tellp();

    for (size_t i = start; i < stop; i++) {

      storage::read_only_ptr<T> block_ptr;
      log_.ptr(archival_tail_, block_ptr);

      auto* metadata = storage::ptr_metadata::get(block_ptr.get().internal_ptr());
      size_t encoded_size = encoder_.encoding_size(metadata->data_size_);
      size_t write_size = sizeof(storage::ptr_metadata) + encoded_size;

      if (file_off + write_size > configuration_params::MAX_ARCHIVAL_FILE_SIZE) {
        archival_out.close();
        archival_out = create_new_archival_file();
        file_off = archival_out.tellp();
      }

      archive_block(archival_out, block_ptr);
      void* archived_data = ALLOCATOR.mmap(cur_file_path(), file_off, encoded_size, storage::state_type::D_ARCHIVED);
      storage::encoded_ptr<T> enc_ptr(archived_data);
      log_.swap_block_ptr(i, enc_ptr);
      file_off = archival_out.tellp();
      archival_tail_ += BLOCK_SIZE;
    }
  }

 private:
  // TODO separate out logic better between archive and archive_block, too many common calls
  /**
   * Write the next block to archival file.
   * @param out archival destination
   * @param file_offset archival file offset to write to
   */
  void archive_block(std::ofstream& out, storage::read_only_ptr<T> block_ptr) {
    // Since the file will be memory-mapped, there must be space in the file for the pointer metadata,
    // so it is written with the data. The actual values of the metadata, apart from the state,
    // do not matter since they'll be overwritten by the allocator.
    auto metadata_copy = *(storage::ptr_metadata::get(block_ptr.get().internal_ptr()));
    metadata_copy.state_ = storage::state_type::D_ARCHIVED;

    size_t encoded_size = encoder_.encoding_size(metadata_copy.data_size_);
    io_utils::write<storage::ptr_metadata>(out, metadata_copy);
    io_utils::write<uint8_t>(out, encoder_.encode(block_ptr.get().decode().get()), encoded_size);
    update_file_header(out, archival_tail_);
  }

  /**
   * Path to current file being used for archival.
   * @return file path
   */
  std::string cur_file_path() {
    return path_ + std::to_string(cur_file_count_) + ".dat";
  }

  /**
   * Create a new archival file and set its header.
   * @return file stream
   */
  std::ofstream create_new_archival_file() {
    cur_file_count_++;
    std::ofstream archival_out(cur_file_path(), std::ofstream::out | std::ofstream::trunc);
    io_utils::write<size_t>(archival_out, archival_tail_);
    io_utils::write<size_t>(archival_out, archival_tail_);
    return archival_out;
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
  int cur_file_count_;
  size_t archival_tail_;

  read_tail rt_;
  monolog_linear<T, MAX_BLOCKS, BLOCK_SIZE, BUF_SIZE>& log_;
  encoder<T> encoder_;

};

}
}

#endif /* DIALOG_MONOLOG_LINEAR_ARCHIVER_H_ */

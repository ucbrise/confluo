#ifndef DIALOG_MONOLOG_LINEAR_ARCHIVER_H_
#define DIALOG_MONOLOG_LINEAR_ARCHIVER_H_

#include "dialog_allocator.h"
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

template<typename T, size_t MAX_BLOCKS, size_t BLOCK_SIZE, size_t BUF_SIZE,
         size_t ARCHIVED_BLOCK_SIZE = BLOCK_SIZE + BUF_SIZE>
class monolog_linear_archiver {

 public:
  typedef T* (*transform_fn)(T* ptr);

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
                          monolog_linear<T, MAX_BLOCKS, BLOCK_SIZE, BUF_SIZE>* log,
                          transform_fn transform = identity_fn) :
    path_(path + "/" + name + "/"),
    cur_file_count_(0),
    archival_tail_(0),
    rt_(rt),
    log_(log),
    transform_(transform) {
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

    std::ofstream archival_out(cur_file_path());
    archival_out.seekp(0, std::ios::end);
    size_t file_off = archival_out.tellp();

    for (size_t i = start; i < stop; i++) {
      if (file_off + BLOCK_WRITE_SIZE > configuration_params::MAX_ARCHIVAL_FILE_SIZE) {
        archival_out.close();
        archival_out = create_new_archival_file();
        file_off = archival_out.tellp();
      }
      archive_block(i, archival_out, file_off);
      file_off += BLOCK_WRITE_SIZE;
      archival_tail_ += BLOCK_SIZE;
    }
  }

 private:
  /**
   * Write a block to archival file and swap
   * the corresponding monolog block pointer.
   * @param block monolog block index
   * @param out archival destination
   * @param file_offset archival file offset to write to
   */
  void archive_block(size_t block, std::ofstream& out, size_t file_offset) {
    storage::read_only_ptr<T> block_ptr;
    log_->ptr(archival_tail_, block_ptr);

    storage::ptr_metadata metadata_copy = *(storage::ptr_metadata::get(block_ptr.get()));
    metadata_copy.state_ = storage::state_type::D_ARCHIVED;

    io_utils::write<storage::ptr_metadata>(out, metadata_copy);
    io_utils::write<T>(out, transform_(block_ptr.get()), ARCHIVED_BLOCK_SIZE);
    update_file_header(out, archival_tail_);

    T* data = ALLOCATOR.mmap<T>(cur_file_path(), file_offset, ARCHIVED_BLOCK_SIZE,
                                storage::state_type::D_ARCHIVED);
    log_->swap_block_ptr(block, data);
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
   * @return
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

  /**
   * Identity transform function.
   * @param block_ptr pointer to block
   * @return block_ptr with no change
   */
  static T* identity_fn(T* block_ptr) {
    return block_ptr;
  }

  std::string path_;
  int cur_file_count_;
  size_t archival_tail_;

  read_tail rt_;
  monolog_linear<T, MAX_BLOCKS, BLOCK_SIZE, BUF_SIZE>* log_;
  transform_fn transform_;

  static const size_t BLOCK_WRITE_SIZE = ARCHIVED_BLOCK_SIZE * sizeof(T) + sizeof(storage::ptr_metadata);

};

}
}

#endif /* DIALOG_MONOLOG_LINEAR_ARCHIVER_H_ */

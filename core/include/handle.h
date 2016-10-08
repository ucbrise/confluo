#ifndef SLOG_HANDLE_H_
#define SLOG_HANDLE_H_

#include "logstore.h"

namespace slog {

struct handle {
 public:
  handle(log_store& handle, uint64_t request_batch_size = 256,
         uint64_t data_block_size = 256 * 64)
      : handle_(handle) {
    id_block_size_ = request_batch_size;
    remaining_ids_ = 0;
    cur_id_ = 0;

    data_block_size_ = data_block_size;
    remaining_bytes_ = 0;
    cur_offset_ = 0;
  }

  uint64_t insert(const unsigned char* record, uint16_t record_len,
                  const token_list& tkns) {
    if (remaining_ids_ == 0) {
      cur_id_ = handle_.olog_->request_id_block(id_block_size_);
      remaining_ids_ = id_block_size_;
    }

    if (remaining_bytes_ < record_len) {
      cur_offset_ = handle_.request_bytes(data_block_size_);
      remaining_bytes_ = data_block_size_;
    }

    handle_.append_record(record, record_len, cur_offset_);
    handle_.update_indexes(cur_id_, tkns);
    handle_.olog_->set(cur_id_, cur_offset_, record_len);
    handle_.olog_->end(cur_id_);
    remaining_ids_--;
    cur_offset_ += record_len;
    return ++cur_id_;
  }

  const bool get(unsigned char* record, const int64_t record_id) {
    return handle_.get(record, record_id);
  }

 private:
  uint64_t data_block_size_;
  uint64_t id_block_size_;

  uint64_t cur_id_;
  uint64_t remaining_ids_;

  uint64_t cur_offset_;
  uint64_t remaining_bytes_;

  log_store& handle_;
};

}

#endif /* SLOG_HANDLE_H_ */

#include "container/cursor/record_cursors.h"

namespace confluo {

distinct_record_cursor::distinct_record_cursor(std::unique_ptr<record_cursor> r_cursor, size_t batch_size)
    : record_cursor(batch_size),
      r_cursor_(std::move(r_cursor)) {
  init();
}

size_t distinct_record_cursor::load_next_batch() {
  size_t i = 0;
  for (; i < current_batch_.size() && r_cursor_->has_more(); ++i, r_cursor_->advance()) {
    record_t const &rec = r_cursor_->get();
    if (seen_.find(rec.log_offset()) == seen_.end()) {
      current_batch_[i] = rec;
      seen_.insert(rec.log_offset());
    } else {
      --i;
    }
  }
  return i;
}

std::unique_ptr<record_cursor> make_distinct(std::unique_ptr<record_cursor> r_cursor, size_t batch_size) {
  return std::unique_ptr<record_cursor>(new distinct_record_cursor(std::move(r_cursor), batch_size));
}

filter_record_cursor::filter_record_cursor(std::unique_ptr<offset_cursor> o_cursor,
                                           const data_log *dlog,
                                           const schema_t *schema,
                                           const parser::compiled_expression &cexpr,
                                           size_t batch_size)
    : record_cursor(batch_size),
      o_cursor_(std::move(o_cursor)),
      dlog_(dlog),
      schema_(schema),
      cexpr_(cexpr) {
  init();
}

size_t filter_record_cursor::load_next_batch() {
  size_t i = 0;
  for (; i < current_batch_.size() && o_cursor_->has_more();
         ++i, o_cursor_->advance()) {
    uint64_t o = o_cursor_->get();
    read_only_data_log_ptr ptr;
    dlog_->cptr(o, ptr);
    if (!cexpr_.test(current_batch_[i] = schema_->apply(o, ptr))) {
      i--;
    }
  }
  return i;
}

}
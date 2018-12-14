#include "container/cursor/json_cursors.h"

namespace confluo {

json_string_cursor::json_string_cursor(std::unique_ptr<record_cursor> r_cursor,
                                           const schema_t *schema,
                                           size_t batch_size)
    : json_cursor(batch_size),
      r_cursor_(std::move(r_cursor)),
      schema_(schema) {
  init();
}

size_t json_string_cursor::load_next_batch() {
  namespace pt = boost::property_tree;
  pt::ptree root;

  size_t i = 0;
  for (; i < current_batch_.size() && r_cursor_->has_more();
         ++i, r_cursor_->advance()) {
    record_t r = r_cursor_->get();
    std::string json_rec = schema_.data_to_json_string(r.data());
    current_batch_[i] = json_rec;
  }
  return i;
}

}
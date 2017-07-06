#ifndef DIALOG_COLUMN_H_
#define DIALOG_COLUMN_H_

#include <string>
#include <cstdint>

#include "index_state.h"

namespace dialog {

struct column_t {
  uint16_t idx;
  data_type type;
  uint16_t offset;
  index_state_t idx_state;
  char name[256];

  column_t()
      : idx(UINT16_MAX),
        type(),
        offset(UINT16_MAX) {
  }

  column_t(const column_t& other) {
    idx = other.idx;
    type = other.type;
    offset = other.offset;
    idx_state = other.idx_state;
    set_name(std::string(other.name));
  }

  void set_name(const std::string& n) {
    std::string tmp = string_utils::to_upper(n);
    size_t size = std::min(n.length(), static_cast<size_t>(255));
    strncpy(name, tmp.c_str(), size);
    name[size] = '\0';
  }

  bool is_indexed() {
    return idx_state.is_indexed();
  }

  bool set_indexing() {
    return idx_state.set_indexing();
  }

  void set_indexed(uint16_t index_id) {
    idx_state.set_indexed(index_id);
  }

  void set_unindexed() {
    idx_state.set_unindexed();
  }

  bool disable_indexing() {
    return idx_state.disable_indexing();
  }

  field_t apply(const void* data) {
    field_t f;
    f.idx = idx;
    f.value.type = type;
    f.value.data = (void*) ((unsigned char*) data + offset);
    f.indexed = is_indexed();
    f.index_id = idx_state.id;
    return f;
  }
};

}

#endif /* DIALOG_COLUMN_H_ */

#ifndef DIALOG_COLUMN_H_
#define DIALOG_COLUMN_H_

#include <string>
#include <cstdint>

#include "value.h"
#include "field.h"
#include "index_state.h"
#include "string_utils.h"

namespace dialog {

struct column_t {
 public:
  uint16_t idx_;
  data_type type_;
  uint16_t offset_;
  index_state_t idx_state_;
  char name_[256];

  column_t()
      : idx_(UINT16_MAX),
        type_(),
        offset_(UINT16_MAX) {
  }

  column_t(uint16_t idx, uint16_t offset, const data_type& type,
           const std::string& name)
      : idx_(idx),
        type_(type),
        offset_(offset) {
    set_name(name);
  }

  column_t(const column_t& other) {
    idx_ = other.idx_;
    type_ = other.type_;
    offset_ = other.offset_;
    idx_state_ = other.idx_state_;
    set_name(std::string(other.name_));
  }

  std::string name() const {
    return std::string(name_);
  }

  const data_type& type() const {
    return type_;
  }

  uint16_t offset() const {
    return offset_;
  }

  uint16_t idx() const {
    return idx_;
  }

  uint16_t index_id() const {
    return idx_state_.get_id();
  }

  bool is_indexed() const {
    return idx_state_.is_indexed();
  }

  bool set_indexing() {
    return idx_state_.set_indexing();
  }

  void set_indexed(uint16_t index_id) {
    idx_state_.set_indexed(index_id);
  }

  void set_unindexed() {
    idx_state_.set_unindexed();
  }

  bool disable_indexing() {
    return idx_state_.disable_indexing();
  }

  field_t apply(const void* data) const {
    return field_t(idx_, type_,
                   reinterpret_cast<const unsigned char*>(data) + offset_,
                   is_indexed(), idx_state_.id);
  }

 private:
  void set_name(const std::string& n) {
    std::string tmp = string_utils::to_upper(n);
    size_t size = std::min(n.length(), static_cast<size_t>(255));
    strncpy(name_, tmp.c_str(), size);
    name_[size] = '\0';
  }
};

}

#endif /* DIALOG_COLUMN_H_ */

#ifndef LIBDIALOG_DIALOG_INDEX_FILTER_H_
#define LIBDIALOG_DIALOG_INDEX_FILTER_H_

#include "byte_string.h"

namespace dialog {

class index_filter {
 public:
  index_filter(uint32_t index_id, const byte_string& key_begin,
               const byte_string& key_end)
      : index_id_(index_id),
        key_begin_(key_begin),
        key_end_(key_end) {
  }

  void set_keys(const byte_string& begin, const byte_string& end) {
    key_begin_ = begin;
    key_end_ = end;
  }

  uint32_t index_id() const {
    return index_id_;
  }

  byte_string const& kbegin() const {
    return key_begin_;
  }

  byte_string const& kend() const {
    return key_end_;
  }

  std::string to_string() const {
    return "range(" + key_begin_.to_string() + "," + key_end_.to_string() + ")"
        + " on index_id=" + std::to_string(index_id_);
  }

 private:
  uint32_t index_id_;
  byte_string key_begin_;
  byte_string key_end_;
};

}

#endif /* LIBDIALOG_DIALOG_INDEX_FILTER_H_ */

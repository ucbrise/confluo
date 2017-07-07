#ifndef LIBDIALOG_DIALOG_FIELD_H_
#define LIBDIALOG_DIALOG_FIELD_H_

namespace dialog {

struct field_t {
  field_t(uint16_t idx, const data_type& type, const void* data, bool indexed,
          uint16_t index_id)
      : idx_(idx),
        value_(type, data),
        indexed_(indexed),
        index_id_(index_id) {
  }

  uint16_t idx() const {
    return idx_;
  }

  const data_type& type() const {
    return value_.type();
  }

  const value_t& value() const {
    return value_;
  }

  bool is_indexed() const {
    return indexed_;
  }

  uint16_t index_id() const {
    return index_id_;
  }

  template<typename T>
  uint64_t get_uint64() const {
    return (uint64_t) *(reinterpret_cast<const T*>(value_.data()));
  }

  template<typename T>
  T get() const {
    return *(reinterpret_cast<const T*>(value_.data()));
  }

 private:
  uint16_t idx_;
  value_t value_;
  bool indexed_;
  uint16_t index_id_;

};

}

#endif /* LIBDIALOG_DIALOG_FIELD_H_ */

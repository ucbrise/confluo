#ifndef LIBDIALOG_DIALOG_FIELD_H_
#define LIBDIALOG_DIALOG_FIELD_H_

namespace dialog {

struct field_t {
  uint16_t idx;
  value_t value;
  bool indexed;
  uint16_t index_id;

  template<typename T>
  uint64_t get_uint64() const {
    return (uint64_t) *(reinterpret_cast<T*>(value.data));
  }

  template<typename T>
  T get() const {
    return *(reinterpret_cast<T*>(value.data));
  }

};

}

#endif /* LIBDIALOG_DIALOG_FIELD_H_ */

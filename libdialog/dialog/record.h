#ifndef LIBDIALOG_DIALOG_RECORD_H_
#define LIBDIALOG_DIALOG_RECORD_H_

#include <vector>
#include <cstdint>

namespace dialog {

struct record_t {
  uint64_t timestamp;
  size_t log_offset;
  const void* data;
  size_t len;
  std::vector<field_t> fields;

  const field_t& get(uint16_t idx) const {
    return fields.at(idx);
  }
};

}

#endif /* LIBDIALOG_DIALOG_RECORD_H_ */

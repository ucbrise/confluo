#include "types/immutable_value.h"

namespace confluo {

immutable_value::immutable_value(const data_type &type)
    : type_(type),
      ptr_(nullptr) {
}

immutable_value::immutable_value(const data_type &type, void *data)
    : type_(type),
      ptr_(data) {
}

data_type const &immutable_value::type() const {
  return type_;
}

void const *immutable_value::ptr() const {
  return ptr_;
}

immutable_raw_data immutable_value::to_data() const {
  return immutable_raw_data(ptr_, type_.size);
}

byte_string immutable_value::to_key(double bucket_size) const {
  return type_.key_transform()(to_data(), bucket_size);
}

bool immutable_value::relop(reational_op_id id, const immutable_value &first, const immutable_value &second) {
  if (first.type_ != second.type_)
    THROW(invalid_operation_exception, "Comparing values of different types");
  return first.type_.relop(id)(first.to_data(), second.to_data());
}

bool operator<(const immutable_value &first, const immutable_value &second) {
  return immutable_value::relop(reational_op_id::LT, first, second);
}

bool operator<=(const immutable_value &first, const immutable_value &second) {
  return immutable_value::relop(reational_op_id::LE, first, second);
}

bool operator>(const immutable_value &first, const immutable_value &second) {
  return immutable_value::relop(reational_op_id::GT, first, second);
}

bool operator>=(const immutable_value &first, const immutable_value &second) {
  return immutable_value::relop(reational_op_id::GE, first, second);
}

bool operator==(const immutable_value &first, const immutable_value &second) {
  return immutable_value::relop(reational_op_id::EQ, first, second);
}

bool operator!=(const immutable_value &first, const immutable_value &second) {
  return immutable_value::relop(reational_op_id::NEQ, first, second);
}

std::string immutable_value::to_string() const {
  return type_.name() + "(" + type_.to_string_op()(to_data()) + ")";
}

}
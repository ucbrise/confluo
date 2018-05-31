#include "types/numeric.h"

namespace confluo {

data_type max(const data_type &t1, const data_type &t2) {
  return type_manager::get_type(std::max(t1.id, t2.id));
}
numeric::numeric()
    : type_(primitive_types::NONE_TYPE()) {
}
numeric::numeric(data_type type)
    : type_(type) {
  memcpy(data_, type_.zero(), type_.size);
}
numeric::numeric(bool val)
    : type_(primitive_types::BOOL_TYPE()) {
  as<bool>() = val;
}
numeric::numeric(int8_t val)
    : type_(primitive_types::CHAR_TYPE()) {
  as<int8_t>() = val;
}
numeric::numeric(uint8_t val)
    : type_(primitive_types::UCHAR_TYPE()) {
  as<uint8_t>() = val;
}
numeric::numeric(int16_t val)
    : type_(primitive_types::SHORT_TYPE()) {
  as<int16_t>() = val;
}
numeric::numeric(uint16_t val)
    : type_(primitive_types::USHORT_TYPE()) {
  as<uint16_t>() = val;
}
numeric::numeric(int32_t val)
    : type_(primitive_types::INT_TYPE()) {
  as<int32_t>() = val;
}
numeric::numeric(uint32_t val)
    : type_(primitive_types::UINT_TYPE()) {
  as<uint32_t>() = val;
}
numeric::numeric(int64_t val)
    : type_(primitive_types::LONG_TYPE()) {
  as<int64_t>() = val;
}
numeric::numeric(uint64_t val)
    : type_(primitive_types::ULONG_TYPE()) {
  as<uint64_t>() = val;
}
numeric::numeric(float val)
    : type_(primitive_types::FLOAT_TYPE()) {
  as<float>() = val;
}
numeric::numeric(double val)
    : type_(primitive_types::DOUBLE_TYPE()) {
  as<double>() = val;
}
numeric::numeric(const data_type &type, void *data)
    : type_(type) {
  if (!type_.is_numeric())
    THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
  memcpy(data_, data, type_.size);
}
numeric::numeric(const immutable_value &val)
    : type_(val.type()) {
  if (!type_.is_numeric())
    THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
  memcpy(data_, val.ptr(), type_.size);
}
bool numeric::is_valid() const {
  return !type_.is_none();
}
numeric numeric::parse(const std::string &str, const data_type &type) {
  numeric value(type);
  type.parse_op()(str, value.data_);
  return value;
}
immutable_raw_data numeric::to_data() const {
  return immutable_raw_data(data_, type_.size);
}
data_type const &numeric::type() const {
  return type_;
}
bool numeric::relop(reational_op_id id, const numeric &first, const numeric &second) {
  // Promote to the larger type
  data_type m = max(first.type_, second.type_);
  numeric v1 = cast(first, m);
  numeric v2 = cast(second, m);
  return m.relop(id)(v1.to_data(), v2.to_data());
}
bool operator<(const numeric &first, const numeric &second) {
  return numeric::relop(reational_op_id::LT, first, second);
}
bool operator<=(const numeric &first, const numeric &second) {
  return numeric::relop(reational_op_id::LE, first, second);
}
bool operator>(const numeric &first, const numeric &second) {
  return numeric::relop(reational_op_id::GT, first, second);
}
bool operator>=(const numeric &first, const numeric &second) {
  return numeric::relop(reational_op_id::GE, first, second);
}
bool operator==(const numeric &first, const numeric &second) {
  return numeric::relop(reational_op_id::EQ, first, second);
}
bool operator!=(const numeric &first, const numeric &second) {
  return numeric::relop(reational_op_id::NEQ, first, second);
}
numeric numeric::unaryop(unary_op_id id, const numeric &n) {
  numeric result(n.type());
  result.type_.unaryop(id)(result.data_, n.to_data());
  return result;
}
numeric operator-(const numeric &n) {
  return numeric::unaryop(unary_op_id::NEGATIVE, n);
}
numeric operator+(const numeric &n) {
  return numeric::unaryop(unary_op_id::POSITIVE, n);
}
numeric operator~(const numeric &n) {
  return numeric::unaryop(unary_op_id::BW_NOT, n);
}
numeric numeric::binaryop(binary_op_id id, const numeric &first, const numeric &second) {
  // Promote to the larger type
  data_type m = max(first.type_, second.type_);
  numeric v1 = cast(first, m);
  numeric v2 = cast(second, m);
  numeric result(m);
  m.binaryop(id)(result.data_, v1.to_data(), v2.to_data());
  return result;
}
numeric operator+(const numeric &first, const numeric &second) {
  return numeric::binaryop(binary_op_id::ADD, first, second);
}
numeric operator-(const numeric &first, const numeric &second) {
  return numeric::binaryop(binary_op_id::SUBTRACT, first, second);
}
numeric operator*(const numeric &first, const numeric &second) {
  return numeric::binaryop(binary_op_id::MULTIPLY, first, second);
}
numeric operator/(const numeric &first, const numeric &second) {
  return numeric::binaryop(binary_op_id::DIVIDE, first, second);
}
numeric operator%(const numeric &first, const numeric &second) {
  return numeric::binaryop(binary_op_id::MODULO, first, second);
}
numeric operator&(const numeric &first, const numeric &second) {
  return numeric::binaryop(binary_op_id::BW_AND, first, second);
}
numeric operator|(const numeric &first, const numeric &second) {
  return numeric::binaryop(binary_op_id::BW_OR, first, second);
}
numeric operator^(const numeric &first, const numeric &second) {
  return numeric::binaryop(binary_op_id::BW_XOR, first, second);
}
numeric operator<<(const numeric &first, const numeric &second) {
  return numeric::binaryop(binary_op_id::BW_LSHIFT, first, second);
}
numeric operator>>(const numeric &first, const numeric &second) {
  return numeric::binaryop(binary_op_id::BW_RSHIFT, first, second);
}
numeric &numeric::operator=(const immutable_value &other) {
  type_ = other.type();
  if (!type_.is_numeric())
    THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
  type_.unaryop(unary_op_id::ASSIGN)(data_, other.to_data());
  return *this;
}
numeric &numeric::operator=(bool value) {
  type_ = primitive_types::BOOL_TYPE();
  as<bool>() = value;
  return *this;
}
numeric &numeric::operator=(int8_t value) {
  type_ = primitive_types::CHAR_TYPE();
  as<int8_t>() = value;
  return *this;
}
numeric &numeric::operator=(uint8_t value) {
  type_ = primitive_types::UCHAR_TYPE();
  as<uint8_t>() = value;
  return *this;
}
numeric &numeric::operator=(int16_t value) {
  type_ = primitive_types::SHORT_TYPE();
  as<int16_t>() = value;
  return *this;
}
numeric &numeric::operator=(uint16_t value) {
  type_ = primitive_types::USHORT_TYPE();
  as<uint16_t>() = value;
  return *this;
}
numeric &numeric::operator=(int32_t value) {
  type_ = primitive_types::INT_TYPE();
  as<int32_t>() = value;
  return *this;
}
numeric &numeric::operator=(uint32_t value) {
  type_ = primitive_types::UINT_TYPE();
  as<uint32_t>() = value;
  return *this;
}
numeric &numeric::operator=(int64_t value) {
  type_ = primitive_types::LONG_TYPE();
  as<int64_t>() = value;
  return *this;
}
numeric &numeric::operator=(uint64_t value) {
  type_ = primitive_types::ULONG_TYPE();
  as<uint64_t>() = value;
  return *this;
}
numeric &numeric::operator=(float value) {
  type_ = primitive_types::FLOAT_TYPE();
  as<float>() = value;
  return *this;
}
numeric &numeric::operator=(double value) {
  type_ = primitive_types::DOUBLE_TYPE();
  as<double>() = value;
  return *this;
}
std::string numeric::to_string() const {
  return type_.name() + "(" + type_.to_string_op()(to_data()) + ")";
}
data_type numeric::type() {
  return type_;
}
uint8_t *numeric::data() {
  return data_;
}
numeric cast(const numeric &val, const data_type &type) {
  return cast_ops::instance()[val.type().id][type.id](val);
}
}
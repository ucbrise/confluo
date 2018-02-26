#ifndef CONFLUO_TYPES_NUMERIC_H_
#define CONFLUO_TYPES_NUMERIC_H_

#include "data_type.h"
#include "immutable_value.h"
#include "string_utils.h"

namespace confluo {

class numeric;

/** Helpers for casting numerics **/
static numeric cast(const numeric& val, const data_type& type);

static data_type max(const data_type& t1, const data_type& t2) {
  return type_manager::get_type(std::max(t1.id, t2.id));
}

class numeric {
 public:
  static const size_t MAX_SIZE = sizeof(uint64_t);

  numeric()
      : type_(NONE_TYPE) {
  }

  numeric(data_type type)
      : type_(type) {
    memcpy(data_, type_.zero(), type_.size);
  }

  numeric(bool val)
      : type_(BOOL_TYPE) {
    as<bool>() = val;
  }

  numeric(int8_t val)
      : type_(CHAR_TYPE) {
    as<int8_t>() = val;
  }

  numeric(uint8_t val)
      : type_(UCHAR_TYPE) {
    as<uint8_t>() = val;
  }

  numeric(int16_t val)
      : type_(SHORT_TYPE) {
    as<int16_t>() = val;
  }

  numeric(uint16_t val)
      : type_(USHORT_TYPE) {
    as<uint16_t>() = val;
  }

  numeric(int32_t val)
      : type_(INT_TYPE) {
    as<int32_t>() = val;
  }

  numeric(uint32_t val)
      : type_(UINT_TYPE) {
    as<uint32_t>() = val;
  }

  numeric(int64_t val)
      : type_(LONG_TYPE) {
    as<int64_t>() = val;
  }

  numeric(uint64_t val)
      : type_(ULONG_TYPE) {
    as<uint64_t>() = val;
  }

  numeric(float val)
      : type_(FLOAT_TYPE) {
    as<float>() = val;
  }

  numeric(double val)
      : type_(DOUBLE_TYPE) {
    as<double>() = val;
  }

  numeric(const data_type& type, void* data)
      : type_(type) {
    if (!type_.is_numeric())
      THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
    memcpy(data_, data, type_.size);
  }

  numeric(const immutable_value& val)
      : type_(val.type()) {
    if (!type_.is_numeric())
      THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
    memcpy(data_, val.ptr(), type_.size);
  }

  bool is_valid() const {
    return !type_.is_none();
  }

  static numeric parse(const std::string& str, const data_type& type) {
    numeric value(type);
    type.parse_op()(str, value.data_);
    return value;
  }

  inline immutable_raw_data to_data() const {
    return immutable_raw_data(data_, type_.size);
  }

  data_type const& type() const {
    return type_;
  }

  // Relational operators
  static bool relop(reational_op_id id, const numeric& first,
                    const numeric& second) {
    // Promote to the larger type
    data_type m = max(first.type_, second.type_);
    numeric v1 = cast(first, m);
    numeric v2 = cast(second, m);
    return m.relop(id)(v1.to_data(), v2.to_data());
  }

  friend inline bool operator <(const numeric& first, const numeric& second) {
    return relop(reational_op_id::LT, first, second);
  }

  friend inline bool operator <=(const numeric& first, const numeric& second) {
    return relop(reational_op_id::LE, first, second);
  }

  friend inline bool operator >(const numeric& first, const numeric& second) {
    return relop(reational_op_id::GT, first, second);
  }

  friend inline bool operator >=(const numeric& first, const numeric& second) {
    return relop(reational_op_id::GE, first, second);
  }

  friend inline bool operator ==(const numeric& first, const numeric& second) {
    return relop(reational_op_id::EQ, first, second);
  }

  friend inline bool operator !=(const numeric& first, const numeric& second) {
    return relop(reational_op_id::NEQ, first, second);
  }

  // Arithmetic operations
  static inline numeric unaryop(unary_op_id id, const numeric& n) {
    numeric result(n.type());
    result.type_.unaryop(id)(result.data_, n.to_data());
    return result;
  }

  friend inline numeric operator-(const numeric& n) {
    return unaryop(unary_op_id::NEGATIVE, n);
  }

  friend inline numeric operator+(const numeric& n) {
    return unaryop(unary_op_id::POSITIVE, n);
  }

  friend inline numeric operator~(const numeric& n) {
    return unaryop(unary_op_id::BW_NOT, n);
  }

  static numeric binaryop(binary_op_id id, const numeric& first,
                          const numeric& second) {
    // Promote to the larger type
    data_type m = max(first.type_, second.type_);
    numeric v1 = cast(first, m);
    numeric v2 = cast(second, m);
    numeric result(m);
    m.binaryop(id)(result.data_, v1.to_data(), v2.to_data());
    return result;
  }

  friend inline numeric operator+(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::ADD, first, second);
  }

  friend inline numeric operator-(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::SUBTRACT, first, second);
  }

  friend inline numeric operator*(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::MULTIPLY, first, second);
  }

  friend inline numeric operator/(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::DIVIDE, first, second);
  }

  friend inline numeric operator%(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::MODULO, first, second);
  }

  friend inline numeric operator&(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::BW_AND, first, second);
  }

  friend inline numeric operator|(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::BW_OR, first, second);
  }

  friend inline numeric operator^(const numeric& first, const numeric& second) {
    return binaryop(binary_op_id::BW_XOR, first, second);
  }

  friend inline numeric operator<<(const numeric& first,
                                   const numeric& second) {
    return binaryop(binary_op_id::BW_LSHIFT, first, second);
  }

  friend inline numeric operator>>(const numeric& first,
                                   const numeric& second) {
    return binaryop(binary_op_id::BW_RSHIFT, first, second);
  }

  numeric& operator=(const immutable_value& other) {
    type_ = other.type();
    if (!type_.is_numeric())
      THROW(invalid_cast_exception, "Casting non-numeric to numeric.");
    type_.unaryop(unary_op_id::ASSIGN)(data_, other.to_data());
    return *this;
  }

  numeric& operator=(bool value) {
    type_ = BOOL_TYPE;
    as<bool>() = value;
    return *this;
  }

  numeric& operator=(int8_t value) {
    type_ = CHAR_TYPE;
    as<int8_t>() = value;
    return *this;
  }

  numeric& operator=(uint8_t value) {
    type_ = UCHAR_TYPE;
    as<uint8_t>() = value;
    return *this;
  }

  numeric& operator=(int16_t value) {
    type_ = SHORT_TYPE;
    as<int16_t>() = value;
    return *this;
  }

  numeric& operator=(uint16_t value) {
    type_ = USHORT_TYPE;
    as<uint16_t>() = value;
    return *this;
  }

  numeric& operator=(int32_t value) {
    type_ = INT_TYPE;
    as<int32_t>() = value;
    return *this;
  }

  numeric& operator=(uint32_t value) {
    type_ = UINT_TYPE;
    as<uint32_t>() = value;
    return *this;
  }

  numeric& operator=(int64_t value) {
    type_ = LONG_TYPE;
    as<int64_t>() = value;
    return *this;
  }

  numeric& operator=(uint64_t value) {
    type_ = ULONG_TYPE;
    as<uint64_t>() = value;
    return *this;
  }

  numeric& operator=(float value) {
    type_ = FLOAT_TYPE;
    as<float>() = value;
    return *this;
  }

  numeric& operator=(double value) {
    type_ = DOUBLE_TYPE;
    as<double>() = value;
    return *this;
  }

  template<typename T,
      typename std::enable_if<
          std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type* =
          nullptr>
  T& as() {
    return *reinterpret_cast<T*>(data_);
  }

  template<typename T,
      typename std::enable_if<
          std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type* =
          nullptr>
  const T& as() const {
    return *reinterpret_cast<const T*>(data_);
  }

  std::string to_string() const {
    return type_.name() + "(" + type_.to_string_op()(to_data()) + ")";
  }

 private:
  data_type type_;
  uint8_t data_[MAX_SIZE];
};

// Cast functions for primitive types
using cast_fn = numeric (*)(const numeric& v);

namespace detail {

template<typename IN, typename OUT>
struct cast_helper {
  static numeric cast(const numeric& v) {
    return numeric(static_cast<OUT>(v.as<IN>()));
  }
};

template<typename T>
struct cast_helper<T, T> {
  static numeric cast(const numeric& v) {
    return v;
  }
};

template<typename OUT>
struct cast_helper<void, OUT> {
  static numeric cast(const numeric& v) {
    throw invalid_cast_exception("Cannot cast none type to any other type");
  }
};

template<typename IN>
struct cast_helper<IN, void> {
  static numeric cast(const numeric& v) {
    throw invalid_cast_exception("Cannot cast any type to none type");
  }
};

template<>
struct cast_helper<void, void> {
  static numeric cast(const numeric& v) {
    throw invalid_cast_exception("Cannot cast none type to none type");
  }
};

}

template<typename IN, typename OUT>
numeric type_cast(const numeric& v) {
  return detail::cast_helper<IN, OUT>::cast(v);
}

template<typename IN>
static std::vector<cast_fn> init_type_cast_ops() {
  return {type_cast<IN, void>, type_cast<IN, bool>, type_cast<IN, int8_t>, type_cast<IN, uint8_t>,
    type_cast<IN, int16_t>, type_cast<IN, uint16_t>, type_cast<IN, int32_t>,
    type_cast<IN, uint32_t>, type_cast<IN, int64_t>, type_cast<IN, uint64_t>,
    type_cast<IN, float>, type_cast<IN, double>};
}

static std::vector<std::vector<cast_fn>> CAST_OPS = {
    init_type_cast_ops<void>(), init_type_cast_ops<bool>(), init_type_cast_ops<
        int8_t>(), init_type_cast_ops<uint8_t>(), init_type_cast_ops<int16_t>(),
    init_type_cast_ops<uint16_t>(), init_type_cast_ops<int32_t>(),
    init_type_cast_ops<uint32_t>(), init_type_cast_ops<int64_t>(),
    init_type_cast_ops<uint64_t>(), init_type_cast_ops<float>(),
    init_type_cast_ops<double>() };

static numeric cast(const numeric& val, const data_type& type) {
  return CAST_OPS[val.type().id][type.id](val);
}

}

#endif /* CONFLUO_TYPES_NUMERIC_H_ */

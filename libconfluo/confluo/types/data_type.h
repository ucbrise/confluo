#ifndef CONFLUO_TYPES_DATA_TYPE_H_
#define CONFLUO_TYPES_DATA_TYPE_H_

#include <limits>
#include <string>
#include <array>
#include <cstdint>
#include <regex>
#include <cstring>

#include "primitive_types.h"
#include "type_properties.h"

namespace confluo {

/**
 * A type of data that operations can be performed on
 */
struct data_type {
 public:
  /** Identifier of the data_type */
  size_t id;
  /** Size items of the type take up in bytes */
  size_t size;

  /**
   * Initializes the data type to have a default identifier and
   * zero bytes for the size
   */
  data_type()
      : id(0),
        size(0) {
  }

  /**
   * Constructs a data type with the specified identifier and size in bytes
   *
   * @param _id The unique identifier for the data type
   * @param _size The size of the data type in bytes
   */
  data_type(uint16_t _id, size_t _size)
      : id(_id),
        size(_size) {
    id = _id;
    size = _size;
  }

  /**
   * Assigns contents of another data type to this data type
   *
   * @param other The other data type, whose contents are used to create
   * this data type
   * @return Reference to this data type
   */
  inline data_type& operator=(const data_type& other) {
    id = other.id;
    size = other.size;
    return *this;
  }

  /**
   * Checks for equality between this data type and another data type
   *
   * @param other The other data type that's used for comparison
   *
   * @return True if the characteristics of the data types match, false
   * otherwise
   */
  inline bool operator==(const data_type& other) const {
    return id == other.id && size == other.size;
  }

  /**
   * Checks for inequality between two data types
   *
   * @param other The other data type that's used for comparison
   *
   * @return True if at least one of the characteristics of the data types
   * don't match, false otherwise
   */
  inline bool operator!=(const data_type& other) const {
    return id != other.id || size != other.size;
  }

  /**
   * Fetches the minimum value for this data type
   *
   * @return A pointer to the minimum value for this data type
   */
  void* min() const {
    return DATA_TYPES[id].min;
  }

  /**
   * Fetches the maximum value for this data type
   *
   * @return A pointer to the maximum value for this data type
   */
  void* max() const {
    return DATA_TYPES[id].max;
  }

  /**
   * Gets the step value for this data type
   *
   * @return A pointer to the step value for this data type
   */
  void* one() const {
    return DATA_TYPES[id].one;
  }

  /**
   * Gets the value that represents zero for this data type
   *
   * @return A pointer to the zero value for this data type
   */
  void* zero() const {
    return DATA_TYPES[id].zero;
  }

  /**
   * Fetches the desired relational operator based on id
   *
   * @param rid The identifier that uniquely identifies the relational
   * operator
   *
   * @return A reference to the relational operator
   */
  inline const relational_op_t& relop(reational_op_id rid) const {
    return DATA_TYPES[id].relational_ops[rid];
  }

  /**
   * Fetches the desired unary operator based on id
   *
   * @param uid The identifier that uniquely identifies the desired unary
   * operator
   *
   * @return A reference to the unary operator
   */
  inline const unary_op_t& unaryop(unary_op_id uid) const {
    return DATA_TYPES[id].unary_ops[uid];
  }

  /**
   * Fetches the desired binary operator based on id
   *
   * @param bid The identifier that uniquely identifies the desired binary
   * operator
   *
   * @return A reference to the desired binary operator
   */
  inline const binary_op_t& binaryop(binary_op_id bid) const {
    return DATA_TYPES[id].binary_ops[bid];
  }

  /**
   * Fetches the key transform operator for this data type
   *
   * @return The key transform operator associated with this data type
   */
  inline const key_op_t& key_transform() const {
    return DATA_TYPES[id].key_transform_op;
  }

  /**
   * Gets the operator that serializes objects of this data type
   *
   * @return A reference to the serialize operator associated with this
   * data type
   */
  inline const serialize_op_t& serialize_op() const {
    return DATA_TYPES[id].serialize_op;
  }

  /**
   * Fetches the operator that deserializes objects of this data type
   *
   * @return A reference to the deserialize operator associated with this
   * data type
   */
  inline const deserialize_op_t& deserialize_op() const {
    return DATA_TYPES[id].deserialize_op;
  }

  /**
   * Gets the operator that parses objects of this data type from strings
   *
   * @return A reference to the parse operator associated with this data
   * type
   */
  inline const parse_op_t& parse_op() const {
    return DATA_TYPES[id].parse_op;
  }

  /**
   * Gets the operator that returns the string representation of objects
   * of this data type
   *
   * @return A reference to the to string operator associated with this
   * data type
   */
  inline const to_string_op_t& to_string_op() const {
    return DATA_TYPES[id].to_string_op;
  }

  /**
   * Checkes whether this data type is valid
   *
   * @return True if this data type is valid, false otherwise
   */
  inline bool is_valid() const {
    return id >= 1 && id < DATA_TYPES.size();
  }

  /**
   * Determines whether this data type is none
   *
   * @return True if this data type is none, false otherwise
   */
  inline bool is_none() const {
    return id == 0;
  }

  /**
   * Determines whether this data type is primitive
   *
   * @return True if this is a primitive data type, false otherwise
   */
  inline bool is_primitive() const {
    return id >= 1 && id <= 8;
  }

  /**
   * Determines whether this data type is numeric
   *
   * @return True if this data type is numeric, false otherwise
   */
  inline bool is_numeric() const {
    return DATA_TYPES[id].is_numeric;
  }

  /**
   * Gets the name of this data type
   *
   * @return The name of this data type
   */
  inline std::string name() const {
    return DATA_TYPES[id].name;
  }

  /**
   * Checks whether this data type is bounded by max and min values
   *
   * @return True if this data type is bounded, false otherwise
   */
  inline bool is_bounded() const {
    return DATA_TYPES[id].min != nullptr && DATA_TYPES[id].max != nullptr;
  }

  /**
   * Serializes information about this data type into an output stream
   *
   * @param out The output stream which contains the data
   */
  inline void serialize(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&id), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
  }

  /**
   * Deserializes data from an input stream to construct this data type
   *
   * @param in The input stream where information about this data type
   * is located
   *
   * @return data_type
   */
  inline static data_type deserialize(std::istream& in) {
    size_t id, size;
    in.read(reinterpret_cast<char*>(&id), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&size), sizeof(size_t));
    return data_type(id, size);
  }

  /**
   * Constructs a data type from a string representation of the type
   *
   * @param str The string representation
   *
   * @return The data type matching the string representation
   */
  inline static data_type from_string(const std::string& str) {
    std::regex type_re("([a-zA-Z_]+)(?:\\(([[:digit:]]*)\\))?");
    std::smatch type_parts;
    if (std::regex_match(str, type_parts, type_re)) {
      std::string name = type_parts[1].str();
      size_t size = 0;
      if (type_parts.size() == 3 && type_parts[2].str() != "") {
        size = std::stoull(type_parts[2].str());
      }
      size_t id = find_type_properties(name);
      if (id == 0) {
        THROW(parse_exception, "Unknown type name " + str);
      }
      return data_type(id, DATA_TYPES[id].size ? DATA_TYPES[id].size : size);
    }
    THROW(parse_exception, "Malformed type name " + str);
  }
};

}

#endif /* CONFLUO_TYPES_DATA_TYPE_H_ */

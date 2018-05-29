#ifndef CONFLUO_TYPES_DATA_TYPE_H_
#define CONFLUO_TYPES_DATA_TYPE_H_

#include <limits>
#include <string>
#include <array>
#include <cstdint>
#include <regex>
#include <cstring>

#include "primitive_types.h"
#include "serde_ops.h"
#include "string_ops.h"
#include "arithmetic_ops.h"
#include "key_ops.h"
#include "relational_ops.h"

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
  data_type();

  /**
   * Constructs a data type with the specified identifier and size in bytes
   *
   * @param _id The unique identifier for the data type
   * @param _size The size of the data type in bytes
   */
  data_type(uint16_t _id, size_t _size);

  /**
   * Constructs a data type with the specified name and size in bytes
   * @param name
   * @param _size
   */
  data_type(const std::string &name, size_t _size);

  /**
   * Assigns contents of another data type to this data type
   *
   * @param other The other data type, whose contents are used to create
   * this data type
   * @return Reference to this data type
   */
  data_type &operator=(const data_type &other);

  /**
   * Checks for equality between this data type and another data type
   *
   * @param other The other data type that's used for comparison
   *
   * @return True if the characteristics of the data types match, false
   * otherwise
   */
  bool operator==(const data_type &other) const;

  /**
   * Checks for inequality between two data types
   *
   * @param other The other data type that's used for comparison
   *
   * @return True if at least one of the characteristics of the data types
   * don't match, false otherwise
   */
  bool operator!=(const data_type &other) const;

  /**
   * Fetches the minimum value for this data type
   *
   * @return A pointer to the minimum value for this data type
   */
  void *min() const;

  /**
   * Fetches the maximum value for this data type
   *
   * @return A pointer to the maximum value for this data type
   */
  void *max() const;

  /**
   * Gets the step value for this data type
   *
   * @return A pointer to the step value for this data type
   */
  void *one() const;

  /**
   * Gets the value that represents zero for this data type
   *
   * @return A pointer to the zero value for this data type
   */
  void *zero() const;

  /**
   * Fetches the desired relational operator based on id
   *
   * @param rid The identifier that uniquely identifies the relational
   * operator
   *
   * @return A reference to the relational operator
   */
  const relational_op_t &relop(reational_op_id rid) const;

  /**
   * Fetches the desired unary operator based on id
   *
   * @param uid The identifier that uniquely identifies the desired unary
   * operator
   *
   * @return A reference to the unary operator
   */
  const unary_op_t &unaryop(unary_op_id uid) const;

  /**
   * Fetches the desired binary operator based on id
   *
   * @param bid The identifier that uniquely identifies the desired binary
   * operator
   *
   * @return A reference to the desired binary operator
   */
  const binary_op_t &binaryop(binary_op_id bid) const;

  /**
   * Fetches the key transform operator for this data type
   *
   * @return The key transform operator associated with this data type
   */
  const key_op_t &key_transform() const;

  /**
   * Gets the operator that serializes objects of this data type
   *
   * @return A reference to the serialize operator associated with this
   * data type
   */
  const serialize_op_t &serialize_op() const;

  /**
   * Fetches the operator that deserializes objects of this data type
   *
   * @return A reference to the deserialize operator associated with this
   * data type
   */
  const deserialize_op_t &deserialize_op() const;

  /**
   * Gets the operator that parses objects of this data type from strings
   *
   * @return A reference to the parse operator associated with this data
   * type
   */
  const parse_op_t &parse_op() const;

  /**
   * Gets the operator that returns the string representation of objects
   * of this data type
   *
   * @return A reference to the to string operator associated with this
   * data type
   */
  const to_string_op_t &to_string_op() const;

  /**
   * Checkes whether this data type is valid
   *
   * @return True if this data type is valid, false otherwise
   */
  bool is_valid() const;

  /**
   * Determines whether this data type is none
   *
   * @return True if this data type is none, false otherwise
   */
  bool is_none() const;

  /**
   * Determines whether this data type is primitive
   *
   * @return True if this is a primitive data type, false otherwise
   */
  bool is_primitive() const;

  /**
   * Determines whether this data type is numeric
   *
   * @return True if this data type is numeric, false otherwise
   */
  bool is_numeric() const;

  /**
   * Gets the name of this data type
   *
   * @return The name of this data type
   */
  std::string name() const;

  /**
   * Checks whether this data type is bounded by max and min values
   *
   * @return True if this data type is bounded, false otherwise
   */
  bool is_bounded() const;

  /**
   * Serializes information about this data type into an output stream
   *
   * @param out The output stream which contains the data
   */
  void serialize(std::ostream &out) const;

  /**
   * Deserializes data from an input stream to construct this data type
   *
   * @param in The input stream where information about this data type
   * is located
   *
   * @return data_type
   */
  static data_type deserialize(std::istream &in);

  /**
   * Constructs a data type from a string representation of the type
   *
   * @param str The string representation
   *
   * @return The data type matching the string representation
   */
  static data_type from_string(const std::string &str);
};

}

#endif /* CONFLUO_TYPES_DATA_TYPE_H_ */

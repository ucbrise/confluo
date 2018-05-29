#ifndef CONFLUO_TYPES_IMMUTABLE_VALUE_H_
#define CONFLUO_TYPES_IMMUTABLE_VALUE_H_

#include <cstdlib>

#include "data_type.h"
#include "type_manager.h"
#include "string_utils.h"

namespace confluo {

/**
 * The immutable value class. Contains data that cannot be modified and
 * associated comparative operations.
 */
class immutable_value {
 public:
  /**
   * Constructs an immutable value based on the given data type
   *
   * @param type The data type to create an immutable value from
   */
  immutable_value(const data_type &type = NONE_TYPE);

  /**
   * Constructs an immutable value from a given data type and pointer to
   * the value of the data
   *
   * @param type The data type of the immutable value
   * @param data The data the immutable value contains
   */
  immutable_value(const data_type &type, void *data);

  /**
   * Gets the data type of this immutable value
   *
   * @return The data type associated with this immutable value
   */
  data_type const &type() const;

  /**
   * Gets the data associated with this immutable value
   * @return Pointer to the data of this immutable value
   */
  void const *ptr() const;

  /**
   * Fetches the raw data of this immutable value
   *
   * @return The raw immutable value data associated with this immutable
   * value
   */
  immutable_raw_data to_data() const;

  /**
   * Transforms this immutable value into a byte string for lookup 
   * operations
   *
   * @param bucket_size The bucket_size of the data
   *
   * @return The byte string for this immutable value
   */
  byte_string to_key(double bucket_size) const;

  // Relational operators
  /**
   * Performs a relational operation from two immutable values
   *
   * @param id The identifier for the relational operator used
   * @param first The first immutable value used for comparison
   * @param second The second immutable value used for comparison
   *
   * @return A boolean value representing the result of the relational
   * operation
   */
  static bool relop(reational_op_id id, const immutable_value &first, const immutable_value &second);

  /**
   * Performs a less than comparison between two immutable values
   *
   * @param first The first immutable value used for comparison
   * @param second The second immutable value used for comparison
   *
   * @return True if the first immutable value is less than the second,
   * false otherwise
   */
  friend bool operator<(const immutable_value &first, const immutable_value &second);

  /**
   * Performs a less than or equal to comparison between two immutable
   * values
   *
   * @param first The first immutable value used for comparison
   * @param second The second immutable value used for comparison
   *
   * @return True if the first immutable value is less than or equal to
   * the second immutable value
   */
  friend bool operator<=(const immutable_value &first, const immutable_value &second);

  /**
   * Performs a greater than comparison between two immutable values
   *
   * @param first The first immutable value used for comparison
   * @param second The second immutable value used for comparison
   *
   * @return True if the first immutable value is greater than the second,
   * false otherwise
   */
  friend bool operator>(const immutable_value &first, const immutable_value &second);

  /**
   * Performs a greater than or equal to comparison between two immutable
   * values
   *
   * @param first The first immutable value used for comparison
   * @param second The second immutable value used for comparison
   *
   * @return True if the first immutable value is less than the second,
   * false otherwise
   */
  friend bool operator>=(const immutable_value &first, const immutable_value &second);

  /**
   * Performs an equality comparison between two immutable values
   *
   * @param first The first immutable value used for comparison
   * @param second The second immutable value used for comparison
   *
   * @return True if the first immutable value is equal to the second,
   * false otherwise
   */
  friend bool operator==(const immutable_value &first, const immutable_value &second);

  /**
   * Performs a not equal comparison between two immutable values
   *
   * @param first The first immutable value used for comparison
   * @param second The second immutable value used for comparison
   *
   * @return True if the first immutable value is not equal to the second,
   * false otherwise
   */
  friend bool operator!=(const immutable_value &first, const immutable_value &second);

  /**
   * Converts immutable value data to a specified type
   *
   * @tparam T The type of data to convert the immutable value to
   *
   * @return The data of type T
   */
  template<typename T>
  T &as() {
    return *reinterpret_cast<T *>(ptr_);
  }

  /**
   * Converts immutable value data to a specified type that is constant
   *
   * @tparam T The type to convert the immutable value data to
   *
   * @return The data of type T that is unmodifiable
   */
  template<typename T>
  const T &as() const {
    return *reinterpret_cast<const T *>(ptr_);
  }

  /**
   * String representation of the immutable value
   *
   * @return The immutable value represented as a formatted string
   */
  std::string to_string() const;

 protected:
  /** The data type */
  data_type type_;
  /** The pointer of the immutable value */
  void *ptr_;
};

}

#endif /* CONFLUO_TYPES_IMMUTABLE_VALUE_H_ */

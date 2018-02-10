package confluo.rpc;

/**
 * Functionality for data types
 */
public class DataType {

  rpc_data_type typeId;
  int size;

  /**
   * Initializes a data type based on id and size
   *
   * @param typeId The id of the data type
   * @param size   The size of the data type
   */
  public DataType(rpc_data_type typeId, int size) {
    this.typeId = typeId;
    this.size = size;
  }

  /**
   * Performs an equality check between this data type and the other
   * data type
   *
   * @param other The other data type for comparison
   * @return True if the two data types are equal, false otherwise
   */
  public boolean equals(DataType other) {
    return other.typeId.getValue() == this.typeId.getValue() && other.size == this.size;
  }

  /**
   * Performs a not equal comparison between two data types
   *
   * @param other The other data type for comparison
   * @return True if the two data types are equal, false otherwise
   */
  boolean notEquals(DataType other) {
    return !this.equals(other);
  }
}

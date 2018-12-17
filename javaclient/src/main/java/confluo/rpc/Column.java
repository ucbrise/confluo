package confluo.rpc;

import java.nio.ByteBuffer;

/**
 * Container of values for a specific type in the schema
 */
public class Column {

  private int offset;
  private DataType dataType;
  private String name;

  /**
   * Initializes a column in the schema
   *
   * @param offset The offset of the column
   * @param dtype  The data type of values in the column
   * @param name   The name of the column
   */
  Column(int offset, DataType dtype, String name) {
    this.offset = offset;
    this.dataType = dtype;
    this.name = name.toUpperCase();
  }

  /**
   * Adds data to the column
   *
   * @param data The data to add
   * @return A field containing the data
   */
  Field apply(ByteBuffer data) {
    return new Field(name, dataType, data, offset);
  }

  /**
   * Pack data into to buffer.
   *
   * @param out Output buffer.
   * @param data Data to write.
   */
  void pack(ByteBuffer out, String data) {
    DataParser.parseFromString(dataType, out, data);
  }

  /**
   * Gets the data type of the column
   *
   * @return The data type
   */
  DataType getDataType() {
    return dataType;
  }

  /**
   * Gets the name of the column
   *
   * @return The name of the column
   */
  public String getName() {
    return name;
  }

  /**
   * Convert to String.
   * @return String representation of column.
   */
  @Override
  public String toString() {
    return name + ":" + dataType;
  }

}

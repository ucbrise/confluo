package confluo.rpc;

import java.nio.ByteBuffer;

/**
 * Container of values for a specific type in the schema
 */
public class Column {

  private int idx;
  private long offset;
  private DataType dataType;
  private String name;

  /**
   * Initializes a column in the schema
   *
   * @param idx    The index of the column
   * @param offset The offset of the column
   * @param dtype  The data type of values in the column
   * @param name   The name of the column
   */
  public Column(int idx, long offset, DataType dtype, String name) {
    this.idx = idx;
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
  public Field apply(ByteBuffer data) {
    byte[] dataArray = data.array();
    byte[] specifiedData = new byte[dataType.size];

    for (int i = 0; i < specifiedData.length; i++) {
      specifiedData[i] = dataArray[(int) (offset + i)];
    }
    ByteBuffer result = ByteBuffer.wrap(specifiedData);
    result.put(specifiedData);
    result.flip();

    return new Field(idx, dataType, result);
  }

  /**
   * Gets the data type of the column
   *
   * @return The data type
   */
  public DataType getDataType() {
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

}

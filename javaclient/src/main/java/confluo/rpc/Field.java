package confluo.rpc;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;

/**
 * Contains data stored as part of a record
 */
public class Field {

  private String fieldName;
  private DataType dataType;
  private ByteBuffer data;
  private int fieldOffset;

  /**
   * Initializes the field to the data passed in
   *
   * @param fieldName  The name of the field
   * @param dataType   The data type of the field
   * @param data       The ByteBuffer containing the field data
   * @param dataOffset The offset of the field in the ByteBuffer.
   */
  public Field(String fieldName, DataType dataType, ByteBuffer data, int dataOffset) {
    this.fieldName = fieldName;
    this.dataType = dataType;
    this.data = data.order(ByteOrder.LITTLE_ENDIAN);
    this.fieldOffset = dataOffset;
  }

  /**
   * Get the field name.
   *
   * @return The field name.
   */
  public String getName() {
    return fieldName;
  }

  /**
   * Get the data type.
   *
   * @return The data type.
   */
  public DataType getType() {
    return dataType;
  }

  /**
   * Get value as a boolean.
   *
   * @return Boolean representation of the data.
   */
  public boolean asBoolean() {
    return data.get(fieldOffset) == 1;
  }

  /**
   * Get value as a character.
   *
   * @return Character representation of the data.
   */
  public char asChar() {
    return data.getChar(fieldOffset);
  }

  /**
   * Get value as a short.
   *
   * @return Short representation of the data.
   */
  public short asShort() {
    return data.getShort(fieldOffset);
  }

  /**
   * Get value as an integer.
   *
   * @return Integer representation of the data.
   */
  public int asInt() {
    return data.getInt(fieldOffset);
  }

  /**
   * Get value as a long.
   *
   * @return Long representation of the data.
   */
  public long asLong() {
    return data.getLong(fieldOffset);
  }

  /**
   * Get value as a float.
   *
   * @return Float representation of the data.
   */
  public float asFloat() {
    return data.getFloat(fieldOffset);
  }

  /**
   * Get value as a double.
   *
   * @return Double representation of the data.
   */
  public double asDouble() {
    return data.getDouble(fieldOffset);
  }

  /**
   * Get value as a String.
   *
   * @return String representation of the data.
   */
  public String asString() {
    String value = StandardCharsets.UTF_8.decode((ByteBuffer) data.position(fieldOffset)).toString();
    data.rewind();
    int endOff = value.indexOf('\0');
    if (endOff != -1)
      return value.substring(0, endOff);
    return value;
  }

  /**
   * Convert field to string.
   *
   * @return String representation of the field.
   */
  @Override
  public String toString() {
    String value = DataParser.parseToString(dataType, (ByteBuffer) data.position(fieldOffset));
    data.rewind();
    return value;
  }
}

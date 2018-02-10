package confluo.rpc;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Contains data stored as part of a record
 */
public class Field {

  private int idx;
  private DataType dataType;
  private ByteBuffer data;

  /**
   * Initializes the field to the data passed in
   *
   * @param idx   The index of the field
   * @param dtype The data type the value of the field contains
   * @param data  The data that the field contains
   */
  public Field(int idx, DataType dtype, ByteBuffer data) {
    this.idx = idx;
    this.dataType = dtype;
    this.data = data;
  }

  /**
   * Unpacks the field to get the data
   *
   * @return The data in the field
   */
  public ByteBuffer unpack() {
    data.order(ByteOrder.LITTLE_ENDIAN);
    return data;
  }

  /**
   * Unpacks the field to get the data
   *
   * @return The data in the field
   */
  public ByteBuffer getData() {
    return data;
  }
}

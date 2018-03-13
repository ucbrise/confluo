package confluo.rpc;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * A collection of values containing different types
 */
public class Record {

  private long logOffset;
  private long size;
  private ByteBuffer data;
  private long version;
  private List<Field> fields;

  /**
   * Initializes a record to the specified values
   *
   * @param logOffset The offset from the log
   * @param data      The data the record should hold
   * @param size      The size of the record in bytes
   */
  public Record(long logOffset, ByteBuffer data, long size) {
    this.logOffset = logOffset;
    this.data = data;
    this.size = size;
    this.version = this.logOffset + this.size;
    this.fields = new ArrayList<>();

  }

  /**
   * Adds a value to the record
   *
   * @param value The value to add
   */
  public void pushBack(Field value) {
    fields.add(value);
  }

  /**
   * Gets a field at a specific index
   *
   * @param idx The index of the desired field
   * @return The desired field
   */
  public Field at(int idx) {
    return fields.get(idx);
  }

  /**
   * Gets the data of the record
   *
   * @return THe data
   */
  public ByteBuffer getData() {
    return data;
  }
}

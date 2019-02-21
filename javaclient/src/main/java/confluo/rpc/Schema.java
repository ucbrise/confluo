package confluo.rpc;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.List;

/**
 * The schema for the data in the atomic multilog
 */
public class Schema {

  private int recordSize;
  private List<Column> columns;

  /**
   * Initializes the schema to the list of columns passed in
   *
   * @param columns The list of columns that make up the schema
   */
  public Schema(List<Column> columns) {
    recordSize = 0;
    this.columns = columns;
    for (Column column : this.columns) {
      this.recordSize += column.getDataType().size;
    }
  }

  /**
   * Adds data to the schema
   *
   * @param data The data to add
   * @return The record
   */
  public Record apply(ByteBuffer data) {
    return new Record(data, this);
  }

  /**
   * Pack record into a ByteBuffer.
   *
   * @param rec The record.
   * @return The packed ByteBuffer.
   */
  ByteBuffer pack(List<String> rec) {
    ByteBuffer buffer = ByteBuffer.allocateDirect(recordSize);
    buffer.order(ByteOrder.LITTLE_ENDIAN);
    int off = 0;
    if (rec.size() == columns.size()) {
      off = 1;
      buffer.putLong(Long.parseLong(rec.get(0)));
    } else if (rec.size() == columns.size() - 1) {
      buffer.putLong(System.nanoTime());
    } else {
      throw new IllegalArgumentException("Invalid number of attributes in record");
    }

    for (int i = off; i < rec.size(); ++i) {
      columns.get(i).pack(buffer, rec.get(i));
    }
    return buffer;
  }

  /**
   * Pack record into a ByteBuffer.
   *
   * @param rec The record.
   * @return The packed ByteBuffer.
   */
  public ByteBuffer pack(String... rec) {
    return pack(true,rec);
  }

  /**
   * Pack record into a ByteBuffer.
   *
   * @param direct pack record into direct buffer if true
   * @param rec The record.
   * @return The packed ByteBuffer.
   */
  public ByteBuffer pack(boolean direct,String... rec) {
    ByteBuffer buffer = direct?ByteBuffer.allocateDirect(recordSize):ByteBuffer.allocate(recordSize);
    buffer.order(ByteOrder.LITTLE_ENDIAN);
    int recordOff = 0;
    if (rec.length == columns.size()) {
      recordOff = 1;
      buffer.putLong(Long.parseLong(rec[0]));
    } else if (rec.length == columns.size() - 1) {
      buffer.putLong(System.nanoTime());
    } else {
      throw new IllegalArgumentException("Invalid number of attributes in record");
    }
    for (int i = 0; i < columns.size() - 1; ++i) {
      columns.get(i + 1).pack(buffer, rec[i + recordOff]);
    }

    buffer.rewind();
    return buffer;
  }

  /**
   * Gets the columns in the schema
   *
   * @return The list of columns
   */
  List<Column> getColumns() {
    return columns;
  }

  /**
   * Gets the record size
   *
   * @return The record size
   */
  public int getRecordSize() {
    return recordSize;
  }
}


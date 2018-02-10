package confluo.rpc;

import java.nio.ByteBuffer;
import java.util.List;

/**
 * The schema for the data in the atomic multilog
 */
public class Schema {

  private long recordSize;
  private List<Column> columns;
  private Record rec;

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
   * @param offset The offset in the record where the data is
   * @param data   The data to add
   * @return The record
   */
  public Record apply(long offset, ByteBuffer data) {
    rec = new Record(offset, data, recordSize);
    for (Column column : this.columns) {
      rec.pushBack(column.apply(rec.getData()));
    }
    return rec;
  }

  /**
   * Gets the columns in the schema
   *
   * @return The list of columns
   */
  public List<Column> getColumns() {
    return columns;
  }

  /**
   * Gets the record size
   *
   * @return The record size
   */
  public long getRecordSize() {
    return recordSize;
  }
}


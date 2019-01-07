package confluo.rpc;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * A collection of values containing different types
 */
public class Record implements Iterable<Field> {

  private List<Field> fields;

  /**
   * Initializes a record to the specified values
   *
   * @param data   The data the record should hold
   * @param schema The schema the record conforms to
   */
  Record(ByteBuffer data, Schema schema) {
    this.fields = new ArrayList<>();
    for (Column column : schema.getColumns()) {
      fields.add(column.apply(data));
    }
  }

  /**
   * Gets a field get a specific index
   *
   * @param idx The index of the desired field
   * @return The desired field
   */
  public Field get(int idx) {
    return fields.get(idx);
  }

  /**
   * Convert record to string.
   *
   * @return String representation of record.
   */
  @Override
  public String toString() {
    StringBuilder out = new StringBuilder("[");
    for (int i = 0; i < fields.size(); ++i) {
      out.append(fields.get(i).toString());
      if (i != fields.size() - 1) {
        out.append(", ");
      }
    }
    out.append("]");
    return out.toString();
  }

  /**
   * Returns an iterator over elements Field elements.
   *
   * @return an Iterator.
   */
  @Override
  public Iterator<Field> iterator() {
    return fields.iterator();
  }
}

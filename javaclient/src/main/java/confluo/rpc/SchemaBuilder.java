package confluo.rpc;

import java.util.ArrayList;
import java.util.List;

/**
 * Builder of a schema for the atomic multilog
 */
public class SchemaBuilder {

  private boolean userProvidedTs;
  private long offset;
  private List<Column> columns;

  /**
   * Initializes a default schema builder
   */
  public SchemaBuilder() {
    this.userProvidedTs = false;
    this.offset = 0;
    this.columns = new ArrayList<>();
    Column timestampCol = new Column(0, 0, DataTypes.ULONG_TYPE, "TIMESTAMP");
    this.columns.add(timestampCol);
    this.offset += DataTypes.ULONG_TYPE.size;
  }

  /**
   * Adds a column to the schema builder
   *
   * @param dtype The data type of the column
   * @param name  The name of the column
   * @return This schema builder with the column added
   */
  public SchemaBuilder addColumn(DataType dtype, String name) {
    if (name.toUpperCase().equals("TIMESTAMP")) {
      userProvidedTs = true;
      if (!dtype.equals(DataTypes.ULONG_TYPE)) {
        throw new IllegalStateException("TIMESTAMP must be of ULONG_TYPE");
      }
      return this;
    }
    Column col = new Column(columns.size(), offset, dtype, name);
    columns.add(col);
    offset += dtype.size;
    return this;
  }

  /**
   * Builds a schema by returning the list of columns
   *
   * @return A list of columns that make up the schema
   */
  public List<Column> build() {
    return columns;
  }
}

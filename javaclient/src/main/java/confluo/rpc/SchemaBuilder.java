package confluo.rpc;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Builder of a schema for the atomic multilog
 */
class SchemaBuilder {

  private int offset;
  private List<Column> columns;

  /**
   * Initializes a default schema builder
   */
  SchemaBuilder() {
    this.offset = 0;
    this.columns = new ArrayList<>();
    Column timestampCol = new Column(0, DataTypes.ULONG_TYPE, "TIMESTAMP");
    this.columns.add(timestampCol);
    this.offset += DataTypes.ULONG_TYPE.size;
  }

  /**
   * Adds a column to the schema builder
   *
   * @param dataType The data type of the column
   * @param name     The name of the column
   */
  void addColumn(DataType dataType, String name) {
    if (name.toUpperCase().equals("TIMESTAMP")) {
      if (!dataType.equals(DataTypes.ULONG_TYPE)) {
        throw new IllegalStateException("TIMESTAMP must be of ULONG_TYPE");
      }
      return;
    }
    Column col = new Column(offset, dataType, name);
    columns.add(col);
    offset += dataType.size;
  }

  /**
   * Builds a schema by returning the list of columns
   *
   * @return A list of columns that make up the schema
   */
  List<Column> build() {
    return columns;
  }

  /**
   * Build schema from String.
   *
   * @param schema String representation of schema.
   * @return The parsed schema.
   */
  static List<Column> fromString(String schema) {
    JsonObject schemaObj = new JsonParser().parse(schema).getAsJsonObject();
    SchemaBuilder builder = new SchemaBuilder();
    for (Map.Entry<String, JsonElement> entry : schemaObj.entrySet()) {
      builder.addColumn(DataType.fromString(entry.getValue().getAsString()), entry.getKey());
    }
    return builder.build();
  }
}

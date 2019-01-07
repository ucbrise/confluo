package confluo.rpc;

import java.util.ArrayList;
import java.util.List;

/**
 * Handler of conversions between RPC and non RPC types
 */
class TypeConversions {

  /**
   * Converts a schema to an rpc schema for the client
   *
   * @param schema The schema to convert
   * @return The rpc schema
   */
  static List<rpc_column> convertToRPCSchema(Schema schema) {
    List<rpc_column> rpcColumns = new ArrayList<>();
    for (Column column : schema.getColumns()) {
      rpc_column rpcCol = new rpc_column(column.getDataType().typeId.getValue(), column.getDataType().size, column.getName());
      rpcColumns.add(rpcCol);
    }
    return rpcColumns;
  }

  /**
   * Converts an rpc schema to a schema for confluo
   *
   * @param rpcSchema The rpc schema to convert
   * @return The schema for confluo
   */
  static Schema convertToSchema(List<rpc_column> rpcSchema) {
    SchemaBuilder builder = new SchemaBuilder();
    for (rpc_column column : rpcSchema) {
      rpc_data_type type = rpc_data_type.findByValue(column.getTypeId());
      builder.addColumn(new DataType(type, column.getTypeSize()), column.getName());
    }
    return new Schema(builder.build());
  }
}


package confluo.rpc;

import java.util.ArrayList;
import java.util.List;

public class RPCTypeConversions {

    public static List<rpc_column> convertToRPCSchema(Schema schema) {
        List<rpc_column> rpcColumns = new ArrayList<>();
        for (Column column : schema.getColumns()) {
            rpc_column rpcCol = new rpc_column(column.getDataType().typeId.getValue(), column.getDataType().size, column.getName());
            rpcColumns.add(rpcCol);
        }
        return rpcColumns;
    }

    public static Schema convertToSchema(List<rpc_column> rpcSchema) {
        SchemaBuilder builder = new SchemaBuilder();
        for (rpc_column column : rpcSchema) {
            rpc_data_type type = rpc_data_type.findByValue(column.get_type_id());
            builder.addColumn(new DataType(type, column.get_type_size()), column.get_name());
        }
        return new Schema(builder.build());
    }
}


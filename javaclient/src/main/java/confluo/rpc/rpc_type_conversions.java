package confluo.rpc;

import java.util.ArrayList;
import java.util.List;

public class rpc_type_conversions {

    public static List<rpc_column> convert_to_rpc_schema(schema schema) {
        List<rpc_column> rpc_columns = new ArrayList<>();
        for (column column : schema.get_columns()) {
            rpc_column rpc_col = new rpc_column(column.get_data_type().type_id.getValue(), column.get_data_type().size, column.get_name());
            rpc_columns.add(rpc_col);
        }
        return rpc_columns;
    }

    public static schema convert_to_schema(List<rpc_column> rpc_schema) {
        schema_builder builder = new schema_builder();
        for (rpc_column column : rpc_schema) {
            rpc_data_type type = rpc_data_type.findByValue(column.get_type_id());
            builder.add_column(new data_type(type, column.get_type_size()), column.get_name());
        }
        return new schema(builder.build());
    }
}


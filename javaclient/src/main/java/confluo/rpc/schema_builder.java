package confluo.rpc;

import java.util.ArrayList;
import java.util.List;

public class schema_builder {

    private boolean user_provided_ts;
    private long offset;
    private List<column> columns;

    public schema_builder() {
        this.user_provided_ts = false;
        this.offset = 0;
        this.columns = new ArrayList<>();
        column timestamp_col = new column(0, 0, data_types.ULONG_TYPE, "TIMESTAMP");
        this.columns.add(timestamp_col);
        this.offset += data_types.ULONG_TYPE.size;
    }

    public schema_builder add_column(data_type dtype, String name) {
        if (name.toUpperCase().equals("TIMESTAMP")) {
            user_provided_ts = true;
            if (!dtype.equals(data_types.ULONG_TYPE)) {

            }
            return this;
        }
        column col = new column(columns.size(), offset, dtype, name);
        columns.add(col);
        offset += dtype.size;
        return this;
    }

    public List<column> build() {
        return columns;
    }
}

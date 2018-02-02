package confluo.rpc;

import java.util.ArrayList;
import java.util.List;

public class SchemaBuilder {

    private boolean userProvidedTs;
    private long offset;
    private List<Column> columns;

    public SchemaBuilder() {
        this.userProvidedTs = false;
        this.offset = 0;
        this.columns = new ArrayList<>();
        Column timestampCol = new Column(0, 0, DataTypes.ULONG_TYPE, "TIMESTAMP");
        this.columns.add(timestampCol);
        this.offset += DataTypes.ULONG_TYPE.size;
    }

    public SchemaBuilder addColumn(DataType dtype, String name) {
        if (name.toUpperCase().equals("TIMESTAMP")) {
            userProvidedTs = true;
            if (!dtype.equals(DataTypes.ULONG_TYPE)) {

            }
            return this;
        }
        Column col = new Column(columns.size(), offset, dtype, name);
        columns.add(col);
        offset += dtype.size;
        return this;
    }

    public List<Column> build() {
        return columns;
    }
}

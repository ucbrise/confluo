package confluo.rpc;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class schema {

    private long record_size;
    private List<column> columns;
    private record rec;

    public schema(List<column> columns) {
        record_size = 0;
        this.columns = columns;
        for (column column : this.columns) {
            this.record_size += column.get_data_type().size;
        }
    }

    public record apply(long offset, ByteBuffer data) {
        rec = new record(offset, data, record_size);
        for (column column : this.columns) {
            rec.push_back(column.apply(rec.get_data()));
        }
        return rec;
    }

    public List<column> get_columns() {
        return columns;
    }

    public long get_record_size() {
        return record_size;
    }
}


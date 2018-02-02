package confluo.rpc;

import java.nio.ByteBuffer;
import java.util.List;

public class Schema {

    private long recordSize;
    private List<Column> columns;
    private Record rec;

    public Schema(List<Column> columns) {
        recordSize = 0;
        this.columns = columns;
        for (Column column : this.columns) {
            this.recordSize += column.getDataType().size;
        }
    }

    public Record apply(long offset, ByteBuffer data) {
        rec = new Record(offset, data, recordSize);
        for (Column column : this.columns) {
            rec.push_back(column.apply(rec.get_data()));
        }
        return rec;
    }

    public List<Column> getColumns() {
        return columns;
    }

    public long getRecordSize() {
        return recordSize;
    }
}


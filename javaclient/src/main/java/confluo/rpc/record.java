package confluo.rpc;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by neil on 1/26/18.
 */
public class record {

    private long log_offset;
    private long size;
    private ByteBuffer data;
    private long version;
    private List<field> fields;

    public record(long log_offset, ByteBuffer data, long size) {
        this.log_offset = log_offset;
        this.data = data;
        this.size = size;
        this.version = this.log_offset + this.size;
        this.fields = new ArrayList<>();

    }

    public void push_back(field value) {
        fields.add(value);
    }

    public field at(int idx) {
        return fields.get(idx);
    }

    public ByteBuffer get_data() {
        return data;
    }
}

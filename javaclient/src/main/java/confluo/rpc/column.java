package confluo.rpc;

import java.nio.ByteBuffer;

public class column {

    private int idx;
    private long offset;
    private data_type data_type;
    private String name;

    public column(int idx, long offset, data_type dtype, String name) {
        this.idx = idx;
        this.offset = offset;
        this.data_type = dtype;
        this.name = name.toUpperCase();

    }

    public field apply(ByteBuffer data) {
        byte[] data_array = data.array();
        byte[] specified_data = new byte[data_type.size];

        for (int i = 0; i < specified_data.length; i++) {
            specified_data[i] = data_array[(int) (offset + i)];
        }
        ByteBuffer result = ByteBuffer.wrap(specified_data);
        result.put(specified_data);
        result.flip();

        return new field(idx, data_type, result);
    }

    public data_type get_data_type() {
        return data_type;
    }

    public String get_name() {
        return name;
    }

}

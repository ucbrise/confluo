package confluo.rpc;

import java.nio.ByteBuffer;

public class Column {

    private int idx;
    private long offset;
    private DataType dataType;
    private String name;

    public Column(int idx, long offset, DataType dtype, String name) {
        this.idx = idx;
        this.offset = offset;
        this.dataType = dtype;
        this.name = name.toUpperCase();

    }

    public Field apply(ByteBuffer data) {
        byte[] data_array = data.array();
        byte[] specified_data = new byte[dataType.size];

        for (int i = 0; i < specified_data.length; i++) {
            specified_data[i] = data_array[(int) (offset + i)];
        }
        ByteBuffer result = ByteBuffer.wrap(specified_data);
        result.put(specified_data);
        result.flip();

        return new Field(idx, dataType, result);
    }

    public DataType getDataType() {
        return dataType;
    }

    public String getName() {
        return name;
    }

}

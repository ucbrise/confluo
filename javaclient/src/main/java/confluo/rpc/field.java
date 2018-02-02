package confluo.rpc;

import java.nio.ByteBuffer;
import java.util.HashMap;

public class Field {

    private int idx;
    private DataType dataType;
    private ByteBuffer data;
    private static HashMap<rpc_data_type, String> FORMAT_CODES;

    static {
        FORMAT_CODES = new HashMap<>();
        FORMAT_CODES.put(TypeId.BOOL, "?");
        FORMAT_CODES.put(TypeId.CHAR, "c");
        FORMAT_CODES.put(TypeId.SHORT, "h");
        FORMAT_CODES.put(TypeId.INT, "i");
        FORMAT_CODES.put(TypeId.LONG, "l");
        FORMAT_CODES.put(TypeId.FLOAT, "f");
        FORMAT_CODES.put(TypeId.DOUBLE, "d");
        FORMAT_CODES.put(TypeId.STRING, "s");
    }

    public Field(int idx, DataType dtype, ByteBuffer data) {
        this.idx = idx;
        this.dataType = dtype;
        this.data = data;
    }

    public byte unpack() {
        rpc_data_type tid = dataType.typeId;
        String formatCode;
        if (tid == TypeId.STRING) {
            formatCode = dataType.size + FORMAT_CODES.get(tid);
        } else {
             formatCode = FORMAT_CODES.get(tid);
        }
        return data.get(0);
    }

    public ByteBuffer getData() {
        return data;
    }
}

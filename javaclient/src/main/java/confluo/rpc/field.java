package confluo.rpc;

import java.nio.ByteBuffer;
import java.util.HashMap;

/**
 * Created by neil on 1/26/18.
 */
public class field {

    private int idx;
    private data_type data_type;
    private ByteBuffer data;
    private static HashMap<rpc_data_type, String> FORMAT_CODES;
    static {
        FORMAT_CODES = new HashMap<>();
        FORMAT_CODES.put(type_id.BOOL, "?");
        FORMAT_CODES.put(type_id.CHAR, "c");
        FORMAT_CODES.put(type_id.SHORT, "h");
        FORMAT_CODES.put(type_id.INT, "i");
        FORMAT_CODES.put(type_id.LONG, "l");
        FORMAT_CODES.put(type_id.FLOAT, "f");
        FORMAT_CODES.put(type_id.DOUBLE, "d");
        FORMAT_CODES.put(type_id.STRING, "s");
    }

    public field(int idx, data_type dtype, ByteBuffer data) {
        this.idx = idx;
        this.data_type = dtype;
        this.data = data;
    }

    byte unpack() {
        rpc_data_type tid = data_type.type_id;
        String format_code;
        if (tid == type_id.STRING) {
            format_code = data_type.size + FORMAT_CODES.get(tid);
        } else {
             format_code = FORMAT_CODES.get(tid);
        }
        return data.get(0);
    }
}

package confluo.rpc;

public class data_types {
    public static data_type BOOL_TYPE = new data_type(type_id.BOOL, 1);
    public static data_type CHAR_TYPE = new data_type(type_id.CHAR, 1);
    public static data_type UCHAR_TYPE = new data_type(type_id.UCHAR, 1);
    public static data_type SHORT_TYPE = new data_type(type_id.SHORT, 2);
    public static data_type USHORT_TYPE = new data_type(type_id.USHORT, 2);
    public static data_type INT_TYPE = new data_type(type_id.INT, 4);
    public static data_type UINT_TYPE = new data_type(type_id.UINT, 4);
    public static data_type LONG_TYPE = new data_type(type_id.LONG, 8);
    public static data_type ULONG_TYPE = new data_type(type_id.ULONG, 8);
    public static data_type FLOAT_TYPE = new data_type(type_id.FLOAT, 4);
    public static data_type DOUBLE_TYPE = new data_type(type_id.DOUBLE, 8);

    public static data_type STRING_TYPE(int size) {
        return new data_type(type_id.STRING, size);
    }
}


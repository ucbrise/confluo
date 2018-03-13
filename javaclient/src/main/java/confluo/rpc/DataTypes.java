package confluo.rpc;

/**
 * Container for all of the primitive data types in Confluo
 */
public class DataTypes {
  public static DataType BOOL_TYPE = new DataType(TypeId.BOOL, 1);
  public static DataType CHAR_TYPE = new DataType(TypeId.CHAR, 1);
  public static DataType UCHAR_TYPE = new DataType(TypeId.UCHAR, 1);
  public static DataType SHORT_TYPE = new DataType(TypeId.SHORT, 2);
  public static DataType USHORT_TYPE = new DataType(TypeId.USHORT, 2);
  public static DataType INT_TYPE = new DataType(TypeId.INT, 4);
  public static DataType UINT_TYPE = new DataType(TypeId.UINT, 4);
  public static DataType LONG_TYPE = new DataType(TypeId.LONG, 8);
  public static DataType ULONG_TYPE = new DataType(TypeId.ULONG, 8);
  public static DataType FLOAT_TYPE = new DataType(TypeId.FLOAT, 4);
  public static DataType DOUBLE_TYPE = new DataType(TypeId.DOUBLE, 8);

  public static DataType STRING_TYPE(int size) {
    return new DataType(TypeId.STRING, size);
  }
}


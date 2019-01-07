package confluo.rpc;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Functionality for data types
 */
public class DataType {

  rpc_data_type typeId;
  int size;

  /**
   * Initializes a data type based on id and size
   *
   * @param typeId The id of the data type
   * @param size   The size of the data type
   */
  DataType(rpc_data_type typeId, int size) {
    this.typeId = typeId;
    this.size = size;
  }

  /**
   * Performs an equality check between this data type and the other
   * data type
   *
   * @param other The other data type for comparison
   * @return True if the two data types are equal, false otherwise
   */
  @Override
  public boolean equals(Object other) {
    if (other instanceof DataType) {
      DataType t = (DataType) other;
      return t.typeId.getValue() == this.typeId.getValue() && t.size == this.size;
    }
    return false;
  }

  @Override
  public String toString() {
    switch (this.typeId) {
      case RPC_NONE:
        return "NONE";
      case RPC_BOOL:
        return "BOOL";
      case RPC_CHAR:
        return "CHAR";
      case RPC_UCHAR:
        return "UCHAR";
      case RPC_SHORT:
        return "SHORT";
      case RPC_USHORT:
        return "USHORT";
      case RPC_INT:
        return "INT";
      case RPC_UINT:
        return "UINT";
      case RPC_LONG:
        return "LONG";
      case RPC_ULONG:
        return "ULONG";
      case RPC_FLOAT:
        return "FLOAT";
      case RPC_DOUBLE:
        return "DOUBLE";
      case RPC_STRING:
        return "STRING(" + this.size + ")";
      default:
        return "UNKNOWN_TYPE";
    }
  }

  static DataType fromString(String typeName) {
    if (typeName.equals("BOOL")) {
      return new DataType(rpc_data_type.RPC_BOOL, 1);
    } else if (typeName.equals("CHAR")) {
      return new DataType(rpc_data_type.RPC_CHAR, 1);
    } else if (typeName.equals("UCHAR")) {
      return new DataType(rpc_data_type.RPC_UCHAR, 1);
    } else if (typeName.equals("SHORT")) {
      return new DataType(rpc_data_type.RPC_SHORT, 2);
    } else if (typeName.equals("USHORT")) {
      return new DataType(rpc_data_type.RPC_USHORT, 2);
    } else if (typeName.equals("INT")) {
      return new DataType(rpc_data_type.RPC_INT, 4);
    } else if (typeName.equals("UINT")) {
      return new DataType(rpc_data_type.RPC_UINT, 4);
    } else if (typeName.equals("LONG")) {
      return new DataType(rpc_data_type.RPC_LONG, 8);
    } else if (typeName.equals("ULONG")) {
      return new DataType(rpc_data_type.RPC_ULONG, 8);
    } else if (typeName.equals("FLOAT")) {
      return new DataType(rpc_data_type.RPC_FLOAT, 4);
    } else if (typeName.equals("DOUBLE")) {
      return new DataType(rpc_data_type.RPC_DOUBLE, 8);
    } else if (typeName.startsWith("STRING")) {
      return new DataType(rpc_data_type.RPC_STRING, Integer.parseInt(typeName.split("\\D+")[1]));
    } else {
      throw new IllegalArgumentException("Unknown data type: " + typeName);
    }
  }
}

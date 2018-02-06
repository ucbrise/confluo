package confluo.rpc;

public class DataType {

    rpc_data_type typeId;
    int size;

    public DataType(rpc_data_type typeId, int size) {
        this.typeId = typeId;
        this.size = size;
    }

    public boolean equals(DataType other) {
        return other.typeId.getValue() == this.typeId.getValue() && other.size == this.size;
    }

    boolean notEquals(DataType other) {
        return !this.equals(other);
    }
}

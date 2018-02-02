package confluo.rpc;

public class DataType {

    rpc_data_type typeId;
    int size;

    public DataType(rpc_data_type typeId, int size) {
        this.typeId = typeId;
        this.size = size;
    }

    public boolean equals(DataType other) {
        if (other.typeId.getValue() == this.typeId.getValue() && other.size == this.size) {
            return true;
        }
        return false;
    }

    boolean notEquals(DataType other) {
        return !this.equals(other);
    }
}

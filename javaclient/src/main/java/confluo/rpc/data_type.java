package confluo.rpc;

public class data_type {

    rpc_data_type type_id;
    int size;

    public data_type(rpc_data_type type_id, int size) {
        this.type_id = type_id;
        this.size = size;
    }

    public boolean equals(data_type other) {
        if (other.type_id.getValue() == this.type_id.getValue() && other.size == this.size) {
            return true;
        }
        return false;
    }

    boolean notEquals(data_type other) {
        return !this.equals(other);
    }
}

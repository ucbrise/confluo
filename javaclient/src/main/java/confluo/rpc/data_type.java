package confluo.rpc;

/**
 * Created by neil on 1/26/18.
 */
public class data_type {

    rpc_data_type type_id;
    int size;

    data_type(rpc_data_type type_id, int size) {
        this.type_id = type_id;
        this.size = size;
    }

    boolean equals(data_type other) {
        if (other.type_id == this.type_id && other.size == this.size) {
            return true;
        }
        return false;
    }

    boolean notEquals(data_type other) {
        return !this.equals(other);
    }
}

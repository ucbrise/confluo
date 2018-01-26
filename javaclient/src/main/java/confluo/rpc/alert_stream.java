package confluo.rpc;

/**
 * Created by neil on 1/26/18.
 */
public class alert_stream {

    private long multilog_id;
    private rpc_service.Client client;
    private byte[] stream;
    private rpc_iterator_handle handle;
    private schema cur_schema;

    public alert_stream(long multilog_id, schema schema, rpc_service.Client client, rpc_iterator_handle handle) {
        this.multilog_id = multilog_id;
        this.cur_schema = schema;
        this.client = client;
        this.handle = handle;
        this.stream = handle.get_data();
    }

    public void iterator() {

    }

    public boolean has_more() {
       return handle.is_has_more() || stream != null;
    }
}

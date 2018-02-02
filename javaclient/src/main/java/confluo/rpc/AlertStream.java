package confluo.rpc;

public class AlertStream {

    private long multilogId;
    private rpc_service.Client client;
    private byte[] stream;
    private rpc_iterator_handle handle;
    private Schema curSchema;

    public AlertStream(long multilogId, Schema schema, rpc_service.Client client, rpc_iterator_handle handle) {
        this.multilogId = multilogId;
        this.curSchema = schema;
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

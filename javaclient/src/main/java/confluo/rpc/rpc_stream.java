package confluo.rpc;

class record_stream {

    private long multilog_id;
    private long cur_off;
    private rpc_service.Client client;
    private rpc_iterator_handle handle;
    private schema schema;

    public record_stream(long multilog_id, schema schema, rpc_service.Client client, rpc_iterator_handle handle)
    {
        this.multilog_id = multilog_id;
        this.schema = schema;
        this.client = client;
        this.handle = handle;
        this.cur_off = 0;
    }

    public void iter() {
        while (has_more()) {
            byte[] data = handle.get_data();

            //schema.apply(0, )
        }
    }

    public boolean has_more() {
       return this.cur_off == -1 || cur_off != handle.get_num_entries();
    }
}


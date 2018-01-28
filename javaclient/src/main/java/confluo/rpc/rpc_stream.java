package confluo.rpc;

import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.Iterator;

class record_stream implements Iterable<record> {

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

    public record next() {
        byte[] data = handle.get_data();
        ByteBuffer handle_data = ByteBuffer.allocate((int) (data.length - cur_off));
        for (int i = (int) cur_off; i < data.length; i++) {
            handle_data.put(data[i]);
        }

        record next = schema.apply(0, handle_data);
        cur_off += schema.get_record_size();
        if (cur_off == handle.get_data().length && handle.is_has_more()) {
            try {
                handle = client.get_more(multilog_id, handle.get_desc());
                cur_off = 0;
            } catch (TException e) {
                e.printStackTrace();
            }
        }

        return next;
    }

    public boolean has_more() {
       return this.cur_off == -1 || cur_off != handle.get_num_entries();
    }

    @Override
    public Iterator<record> iterator() {
        Iterator<record> iterator = new Iterator<record>() {
            @Override
            public boolean hasNext() {
                return has_more();
            }

            @Override
            public record next() {
                return next();
            }

            @Override
            public void remove() {
                throw new UnsupportedOperationException();
            }
        };
        return null;
    }
}


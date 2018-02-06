package confluo.rpc;

import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.Iterator;

public class RecordStream implements Iterable<Record> {

    private long multilogId;
    private long curOff;
    private rpc_service.Client client;
    private rpc_iterator_handle handle;
    private Schema schema;

    RecordStream(long multilogId, Schema schema, rpc_service.Client client, rpc_iterator_handle handle)
    {
        this.multilogId = multilogId;
        this.schema = schema;
        this.client = client;
        this.handle = handle;
        this.curOff = 0;
    }

    private Record next() {
        byte[] data = handle.get_data();
        ByteBuffer handleData = ByteBuffer.allocate((int) (data.length - curOff));
        for (int i = (int) curOff; i < data.length; i++) {
            handleData.put(data[i]);
        }

        Record next = schema.apply(0, handleData);
        curOff += schema.getRecordSize();
        if (curOff == handle.get_data().length && handle.is_has_more()) {
            try {
                handle = client.get_more(multilogId, handle.get_desc());
                curOff = 0;
            } catch (TException e) {
                e.printStackTrace();
            }
        }

        return next;
    }

    private boolean hasMore() {
       return this.curOff == -1 || curOff != handle.get_num_entries();
    }

    @Override
    public Iterator<Record> iterator() {
        return new Iterator<Record>() {
            @Override
            public boolean hasNext() {
                return RecordStream.this.hasMore();
            }

            @Override
            public Record next() {
                return RecordStream.this.next();
            }

            @Override
            public void remove() {
                throw new UnsupportedOperationException();
            }
        };
    }
}


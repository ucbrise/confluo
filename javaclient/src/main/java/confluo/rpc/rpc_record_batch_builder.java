package confluo.rpc;

import java.nio.ByteBuffer;
import java.util.ArrayList;

public class rpc_record_batch_builder {

    static double TIME_BLOCK = 1e6;

    private long num_records;
    private long ts;
    private int time_block;
    private rpc_record_batch batch;
    private ByteBuffer data;

    public rpc_record_batch_builder() {
        num_records = 0;
        clear();
    }

    public void add_record(ByteBuffer record) {
        ts = record.getLong();
        time_block = (int) (ts / TIME_BLOCK);

        batch.get_blocks().get(time_block).set_data(record);
        num_records += 1;

    }

    public rpc_record_batch get_batch() {
        batch = new rpc_record_batch(new ArrayList<rpc_record_block>(), num_records);
        for (long time_block = 0; time_block < batch.get_blocks().size(); time_block++) {
            ByteBuffer data = batch.get_blocks().get((int) time_block).buffer_for_data();
            num_records = batch.get_blocks().get((int) time_block).get_nrecords();
            batch.get_blocks().add(new rpc_record_block(time_block, data, num_records));
        }
        clear();
        return batch;
    }

    public void clear() {
        batch = new rpc_record_batch();
        batch.get_blocks();
    }

}

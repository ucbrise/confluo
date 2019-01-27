package confluo.rpc;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 *
 *  A builder for a batch of message,which different from
 *  @see RecordBatchBuilder ,we don't batch the record,which has same time block,into the same block
 *
 **/
public class MessageBatchBuilder {
    /**
     * TIME_BLOCK: The size of a time block for a record batch
     */
    private static double TIME_BLOCK = 1e6;

    private long numRecords;
    private rpc_record_batch batch;
    private Schema schema;

    /**
     * Initializes an empty rpc record batch builder
     */
    public MessageBatchBuilder(Schema schema) {
        this.numRecords = 0;
        this.schema = schema;
        this.batch= new rpc_record_batch();
    }

    /**
     * Adds a record to the batch builder
     *
     * @param record The record to add to the batch builder
     */
    public void addRecord(ByteBuffer record) throws IOException {
        long ts = record.getLong();
        long timeBlock = (int) (ts / TIME_BLOCK);
        record.position(0);
        batch.addToBlocks(new rpc_record_block(timeBlock, record, 1));
        numRecords += 1;
    }

    /**
     * Gets the record batch
     *
     * @return The record batch containing the records
     */
    public rpc_record_batch getBatch() throws IOException {
        batch.setNrecords(numRecords);
        return batch;
    }

    /**
     * Clears the record batch builder
     */
    public void clear() {
        batch.clear();
        numRecords = 0;
    }
}

package confluo.rpc;

import java.nio.ByteBuffer;
import java.util.ArrayList;

/**
 * A builder for a batch of records
 */
public class RPCRecordBatchBuilder {

  /**
   * TIME_BLOCK: The size of a time block for a record batch
   */
  static double TIME_BLOCK = 1e6;

  private long numRecords;
  private long ts;
  private int timeBlock;
  private rpc_record_batch batch;
  private ByteBuffer data;

  /**
   * Initializes an empty rpc record batch builder
   */
  public RPCRecordBatchBuilder() {
    numRecords = 0;
    clear();
  }

  /**
   * Adds a record to the batch builder
   *
   * @param record The record to add to the batch builder
   */
  public void addRecord(ByteBuffer record) {
    ts = record.getLong();
    timeBlock = (int) (ts / TIME_BLOCK);

    batch.getBlocks().get(timeBlock).setData(record);
    numRecords += 1;

  }

  /**
   * Gets the record batch
   *
   * @return The record batch containing the records
   */
  public rpc_record_batch getBatch() {
    batch = new rpc_record_batch(new ArrayList<rpc_record_block>(), numRecords);
    for (long time_block = 0; time_block < batch.getBlocks().size(); time_block++) {
      ByteBuffer data = batch.getBlocks().get((int) time_block).bufferForData();
      numRecords = batch.getBlocks().get((int) time_block).getNrecords();
      batch.getBlocks().add(new rpc_record_block(time_block, data, numRecords));
    }
    clear();
    return batch;
  }

  /**
   * Clears the record batch builder
   */
  public void clear() {
    batch = new rpc_record_batch();
    batch.getBlocks();
  }

}

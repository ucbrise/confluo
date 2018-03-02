package confluo.rpc;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Map;
import java.util.TreeMap;

/**
 * A builder for a batch of records
 */
public class RPCRecordBatchBuilder {

  /**
   * TIME_BLOCK: The size of a time block for a record batch
   */
  static double TIME_BLOCK = 1e6;

  private long numRecords;
  private TreeMap<Long, ByteArrayOutputStream> batch;
  private Schema schema;

  /**
   * Initializes an empty rpc record batch builder
   */
  public RPCRecordBatchBuilder(Schema schema) {
    this.numRecords = 0;
    this.schema = schema;
  }

  /**
   * Adds a record to the batch builder
   *
   * @param record The record to add to the batch builder
   */
  public void addRecord(ByteBuffer record) throws IOException {
    long ts = record.getLong();
    long timeBlock = (int) (ts / TIME_BLOCK);
    if (!batch.containsKey(timeBlock)) {
      batch.put(timeBlock, new ByteArrayOutputStream());
    }
    batch.get(timeBlock).write(record.array());
    numRecords += 1;

  }

  /**
   * Gets the record batch
   *
   * @return The record batch containing the records
   */
  public rpc_record_batch getBatch() throws IOException {
    rpc_record_batch ret = new rpc_record_batch();
    ret.setNrecords(batch.size());
    for (Map.Entry<Long, ByteArrayOutputStream> entry: batch.entrySet()) {
      long timeBlock = entry.getKey();
      byte[] data = entry.getValue().toByteArray();
      entry.getValue().close();
      long nRecords = data.length / schema.getRecordSize();
      ret.addToBlocks(new rpc_record_block(timeBlock, ByteBuffer.wrap(data), nRecords));
    }
    clear();
    return ret;
  }

  /**
   * Clears the record batch builder
   */
  public void clear() {
    batch.clear();
    numRecords = 0;
  }

}

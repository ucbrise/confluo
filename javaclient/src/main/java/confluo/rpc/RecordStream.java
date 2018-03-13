package confluo.rpc;

import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Iterator;

/**
 * A stream of records and associated functionality
 */
public class RecordStream implements Iterable<Record> {

  private long multilogId;
  private long curOff;
  private rpc_service.Client client;
  private rpc_iterator_handle handle;
  private Schema schema;
  private int index;

  /**
   * Initializes an empty record stream
   *
   * @param multilogId The identifier for the atomic multilog
   * @param schema     The associated schema
   * @param client     The rpc client
   * @param handle     Iterator through the stream
   */
  RecordStream(long multilogId, Schema schema, rpc_service.Client client, rpc_iterator_handle handle) {
    this.multilogId = multilogId;
    this.schema = schema;
    this.client = client;
    this.handle = handle;
    this.curOff = 0;
    this.index = 0;
  }

  /**
   * Iterator next method for the record stream
   *
   * @return A record containing the next element in the stream
   */
  private Record next() {
    byte[] data = handle.getData();
    ByteBuffer handleData = ByteBuffer.allocate((int) (data.length - curOff));
    handleData.order(ByteOrder.LITTLE_ENDIAN);
    for (int i = (int) curOff; i < data.length; i++) {
      handleData.put(data[i]);
    }

    Record next = schema.apply(0, handleData);
    curOff += schema.getRecordSize();
    if (curOff == handle.getData().length && handle.isHasMore()) {
      try {
        handle = client.getMore(multilogId, handle.getDesc());
        curOff = 0;
      } catch (TException e) {
        e.printStackTrace();
      }
    }
    index++;
    return next;
  }

  /**
   * Checks whether the stream has any more elements
   *
   * @return True if there are any more records in the stream, false otherwise
   */
  private boolean hasMore() {
    return index < handle.getNumEntries();
  }

  /**
   * Iterator for the record stream
   *
   * @return Iterator containing hasNext, next, and remove methods for record stream
   */
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


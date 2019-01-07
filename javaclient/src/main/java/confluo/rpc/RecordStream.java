package confluo.rpc;

import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * A stream of records and associated functionality
 */
public class RecordStream implements Iterator<Record> {

  private long multilogId;
  private rpc_service.Client client;
  private rpc_iterator_handle handle;
  private Schema schema;
  private ByteBuffer buf;

  /**
   * Initializes an empty record stream
   *
   * @param mId    The identifier for the atomic multilog
   * @param schema The associated schema
   * @param client The rpc client
   * @param handle Iterator through the stream
   */
  RecordStream(long mId, Schema schema, rpc_service.Client client, rpc_iterator_handle handle) {
    this.multilogId = mId;
    this.schema = schema;
    this.client = client;
    this.handle = handle;
    this.buf = ByteBuffer.wrap(handle.getData());
  }

  /**
   * Iterator next method for the record stream
   *
   * @return A record containing the next element in the stream
   */
  public Record next() {
    if (!buf.hasRemaining()) {
      if (handle.isHasMore()) {
        try {
          handle = client.getMore(multilogId, handle.getDesc());
          buf = ByteBuffer.wrap(handle.getData());
        } catch (TException e) {
          throw new NoSuchElementException("Could not fetch record from server");
        }
      } else {
        throw new NoSuchElementException("Stream has no more elements");
      }
    }
    Record record = schema.apply(buf.slice());
    buf.position(buf.position() + schema.getRecordSize());
    return record;
  }

  /**
   * Checks whether the stream has any more elements
   *
   * @return True if there are any more records in the stream, false otherwise
   */
  public boolean hasNext() {
    return handle.isHasMore() || buf.hasRemaining();
  }
}


package confluo.rpc;

import org.apache.thrift.TException;

import java.util.Iterator;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;

/**
 * A stream of alerts
 */
class AlertStream implements Iterator<String> {

  private long mId;
  private rpc_service.Client client;
  private StringTokenizer stream;
  private rpc_iterator_handle handle;

  /**
   * Initializes a stream of alerts to the data passed in
   *
   * @param mId    The identifier of the atomic multilog
   * @param client The rpc client
   * @param handle The iterator for the stream
   */
  AlertStream(long mId, rpc_service.Client client, rpc_iterator_handle handle) {
    this.mId = mId;
    this.client = client;
    this.handle = handle;
    this.stream = buildReader();
  }

  /**
   * Build a reader from a byte array
   *
   * @return A BufferedReader instance
   */
  private StringTokenizer buildReader() {
    return new StringTokenizer(new String(handle.getData()), "\n");
  }

  /**
   * Gets the next alert in the stream
   *
   * @return The next alert in the stream
   */
  public String next() {
    if (!stream.hasMoreElements()) {
      if (handle.isHasMore()) {
        try {
          handle = client.getMore(mId, handle.getDesc());
        } catch (TException e) {
          throw new NoSuchElementException("Failed to readRaw data from server");
        }
        stream = buildReader();
      } else {
        throw new NoSuchElementException("No more elements");
      }
    }
    return stream.nextElement().toString();
  }

  /**
   * Checks whether the stream has any more elements
   *
   * @return True if there are any elements left in the stream, false otherwise
   */
  public boolean hasNext() {
    return handle.isHasMore() || stream.hasMoreElements();
  }
}

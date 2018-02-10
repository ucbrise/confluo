package confluo.rpc;

import org.apache.thrift.TException;

import java.util.Iterator;

/**
 * A stream of alerts
 */
public class AlertStream implements Iterable<String> {

  private long multilogId;
  private rpc_service.Client client;
  private byte[] stream;
  private rpc_iterator_handle handle;

  /**
   * Initializes a stream of alerts to the data passed in
   *
   * @param multilogId The identifier of the atomic multilog
   * @param client     The rpc client
   * @param handle     The iterator for the stream
   */
  public AlertStream(long multilogId, rpc_service.Client client, rpc_iterator_handle handle) {
    this.multilogId = multilogId;
    this.client = client;
    this.handle = handle;
    this.stream = handle.get_data();
  }

  /**
   * Gets the next alert in the stream
   *
   * @return The next alert in the stream
   * @throws TException Cannot get next alert
   */
  private String next() throws TException {
    int endIdx = 0;
    for (int i = 0; i < stream.length; i++) {
      if (stream[i] == '\n') {
        endIdx = i;
        break;
      }
    }
    byte[] alertBytes = new byte[endIdx];
    for (int i = 0; i < alertBytes.length; i++) {
      alertBytes[i] = stream[i];
    }

    String alert = new String(alertBytes);
    byte[] newStream = new byte[stream.length - endIdx];
    for (int i = 0; i < newStream.length; i++) {
      newStream[i] = stream[i];
    }

    stream = newStream;
    if (!alert.equals("") && handle.is_has_more()) {
      handle = client.get_more(multilogId, handle.get_desc());
      stream = handle.get_data();
    }

    return alert;
  }

  /**
   * Checks whether the stream has any more elements
   *
   * @return True if there are any elements left in the stream, false otherwise
   */
  private boolean hasMore() {
    return handle.is_has_more() || stream != null;
  }

  /**
   * Iterator for the alert stream
   *
   * @return Iterator containing hasNext, next, and remove methods for alert stream
   */
  public Iterator<String> iterator() {
    return new Iterator<String>() {
      @Override
      public boolean hasNext() {
        return hasMore();
      }

      @Override
      public String next() {
        return next();
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException();
      }
    };
  }
}

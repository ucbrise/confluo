package confluo.rpc;

import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransport;

import java.nio.ByteBuffer;
import java.util.List;

/**
 * Client for Confluo through RPC
 */
public class RPCClient {

  private TTransport transport;
  private rpc_service.Client client;
  private long curMultilogId;
  private rpc_atomic_multilog_info info;
  private Schema curSchema;

  /**
   * Initializes the rpc client to the specified host and port
   *
   * @param host The host for the client
   * @param port The port number to communicate through
   * @throws TException Cannot connect to host
   */
  public RPCClient(String host, int port) throws TException {
    connect(host, port);
    curMultilogId = -1;
  }

  /**
   * Closes the rpc client
   *
   * @throws TException Cannot disconnect
   */
  public void close() throws TException {
    disconnect();
  }

  /**
   * Connects the rpc client to the specified host and port
   *
   * @param host The host of the client
   * @param port The port number to communicate through
   * @throws TException Cannot connect
   */
  public void connect(String host, int port) throws TException {
    transport = new TSocket(host, port);
    TBinaryProtocol protocol = new TBinaryProtocol(transport);
    client = new rpc_service.Client(protocol);
    transport.open();
    client.registerHandler();
  }

  /**
   * Disconnects the rpc client from the host and port
   *
   * @throws TException Cannot disconnect
   */
  public void disconnect() throws TException {
    if (transport.isOpen()) {
      client.deregisterHandler();
      transport.close();
    }
  }

  /**
   * Creates an atomic multilog for this client
   *
   * @param atomicMultilogName The name of the atomic multilog to create
   * @param schema             The schema for the atomic multilog
   * @param storageMode        The mode for storage
   * @throws TException Cannot create the atomic multilog
   */
  public void createAtomicMultilog(String atomicMultilogName, Schema schema, rpc_storage_mode storageMode) throws TException {
    this.curSchema = schema;
    List<rpc_column> rpcSchema = RPCTypeConversions.convertToRPCSchema(schema);
    curMultilogId = client.createAtomicMultilog(atomicMultilogName, rpcSchema, storageMode);
  }

  /**
   * Sets the atomic multilog to the desired atomic multilog
   *
   * @param atomicMultilogName The name of atomic multilog to set the current atomic multilog to
   * @throws TException Cannot set the atomic multilog
   */
  public void setCurrentAtomicMultilog(String atomicMultilogName) throws TException {
    info = client.getAtomicMultilogInfo(atomicMultilogName);
    curSchema = RPCTypeConversions.convertToSchema(info.getSchema());
    curMultilogId = info.getId();
  }

  /**
   * Removes an atomic multilog from the client
   *
   * @throws TException Cannot remove the atomic multilog
   */
  public void removeAtomicMultilog() throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    client.removeAtomicMultilog(curMultilogId);
    curMultilogId = -1;
  }

  /**
   * Adds an index to the atomic multilog
   *
   * @param fieldName  The name of the associated field
   * @param bucketSize The bucket in the multilog to add the index
   * @throws TException Cannot add the index
   */
  public void addIndex(String fieldName, double bucketSize) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    client.addIndex(curMultilogId, fieldName, bucketSize);
  }

  /**
   * Removes an index from the atomic multilog
   *
   * @param fieldName The name of the associated field
   * @throws TException Cannot remove the index
   */
  public void removeIndex(String fieldName) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    client.removeIndex(curMultilogId, fieldName);
  }

  /**
   * Adds a filter to the atomic multilog
   *
   * @param filterName The name of the filter
   * @param filterExpr The filter expression
   * @throws TException Cannot add filter
   */
  public void addFilter(String filterName, String filterExpr) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    client.addFilter(curMultilogId, filterName, filterExpr);
  }

  /**
   * Removes a filter from the atomic multilog
   *
   * @param filterName The name of the filter
   * @throws TException Cannot remove the filter
   */
  public void removeFilter(String filterName) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    client.removeFilter(curMultilogId, filterName);
  }

  /**
   * Adds an aggregate to the atomic multilog
   *
   * @param aggregateName The name of the aggregate
   * @param filterName    The name of the filter
   * @param aggregateExpr The aggregate expression
   * @throws TException Cannot add the aggregate
   */
  public void addAggregate(String aggregateName, String filterName, String aggregateExpr) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    client.addAggregate(curMultilogId, aggregateName, filterName, aggregateExpr);
  }

  /**
   * Removes an aggregate from the atomic multilog
   *
   * @param aggregateName The name of the aggregate
   * @throws TException Cannot remove the aggregate
   */
  public void removeAggregate(String aggregateName) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    client.removeAggregate(curMultilogId, aggregateName);
  }

  /**
   * Adds a trigger to the atomic multilog
   *
   * @param triggerName The name of the trigger to add
   * @param triggerExpr The trigger expression
   * @throws TException Cannot add the trigger
   */
  public void installTrigger(String triggerName, String triggerExpr) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    client.addTrigger(curMultilogId, triggerName, triggerExpr);
  }

  /**
   * Removes a trigger from the atomic multilog
   *
   * @param triggerName The name of the trigger
   * @throws TException Cannot remove the trigger
   */
  public void removeTrigger(String triggerName) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    client.removeTrigger(curMultilogId, triggerName);
  }

  /**
   * Gets a new record batch builder
   *
   * @return The RPC record batch builder
   */
  public RPCRecordBatchBuilder getBatchBuilder() {
    return new RPCRecordBatchBuilder(curSchema);
  }

  /**
   * Writes a record to the atomic multilog
   *
   * @param record The record to write
   * @throws TException Cannot append the record
   */
  public void append(ByteBuffer record) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    if (record.array().length != curSchema.getRecordSize()) {
      throw new IllegalStateException("Record size incorrect; expected=" + curSchema.getRecordSize()
          + ", got=" + record.array().length);
    }
    client.append(curMultilogId, record);
  }

  /**
   * Reads data from a specified offset
   *
   * @param offset The offset from the log to read from
   * @return The data at the offset
   * @throws TException Cannot read the record
   */
  public ByteBuffer read(long offset) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    return client.read(curMultilogId, offset, 1);
  }

  /**
   * Gets an aggregate from the atomic multilog
   *
   * @param aggregateName The name of the aggregate
   * @param beginMs       The beginning time in milliseconds
   * @param endMs         The end time in milliseconds
   * @return The aggregate
   * @throws TException Cannot get the aggregate
   */
  public String getAggregate(String aggregateName, long beginMs, long endMs) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    return client.queryAggregate(curMultilogId, aggregateName, beginMs, endMs);
  }

  /**
   * Executes a specified filter
   *
   * @param filterExpr The filter expression
   * @return Record stream containing the data
   * @throws TException Cannot execute the filter
   */
  public RecordStream executeFilter(String filterExpr) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    rpc_iterator_handle handle = client.adhocFilter(curMultilogId, filterExpr);
    return new RecordStream(curMultilogId, curSchema, client, handle);
  }

  /**
   * Queries a filter
   *
   * @param filterName The name of the filter
   * @param beginMs    The beginning time in milliseconds
   * @param endMs      The end time in milliseconds
   * @param filterExpr The filter expression
   * @return A record stream containing the results of the filter
   * @throws TException Cannot query the filter
   */
  public RecordStream queryFilter(String filterName, long beginMs, long endMs, String filterExpr) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    if (filterExpr.equals("")) {
      rpc_iterator_handle handle = client.predefFilter(curMultilogId, filterName, beginMs, endMs);
      return new RecordStream(curMultilogId, curSchema, client, handle);
    } else {
      rpc_iterator_handle handle = client.combinedFilter(curMultilogId, filterName, filterExpr, beginMs, endMs);
      return new RecordStream(curMultilogId, curSchema, client, handle);
    }
  }

  /**
   * Gets the alerts
   *
   * @param beginMs     The beginning time in milliseconds
   * @param endMs       The end time in milliseconds
   * @param triggerName The name of the trigger
   * @return A stream of alerts
   * @throws TException Cannot get the alerts
   */
  public AlertStream getAlerts(long beginMs, long endMs, String triggerName) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    if (triggerName.equals("")) {
      rpc_iterator_handle handle = client.alertsByTime(curMultilogId, beginMs, endMs);
      return new AlertStream(curMultilogId, client, handle);
    } else {
      rpc_iterator_handle handle = client.alertsByTime(curMultilogId, beginMs, endMs);
      return new AlertStream(curMultilogId, client, handle);
    }
  }

  /**
   * Gets the number of records in the atomic multilog
   *
   * @return The number of records
   * @throws TException Cannot get the number of records
   */
  public long numRecords() throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    return client.numRecords(curMultilogId);
  }
}

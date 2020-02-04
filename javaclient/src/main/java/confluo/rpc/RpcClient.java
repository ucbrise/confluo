package confluo.rpc;

import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransport;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * \mainpage Java Client Documentation
 * <p>
 * The library provides a client implementation, the \ref RpcClient that can communicate with the Confluo server.
 * <p>
 * The library internally relies on <a href="http://thrift.apache.org/">Apache Thrift</a> to facilitate the RPCs.
 */

/**
 * Client for Confluo through RPC
 */
public class RpcClient {

  private TTransport transport;
  private rpc_service.Client client;
  private long curMultilogId;
  private Schema curSchema;

  /**
   * Initializes the rpc client to the specified host and port
   *
   * @param host The host for the client
   * @param port The port number to communicate through
   * @throws TException Cannot connect to host
   */
  public RpcClient(String host, int port) throws TException {
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
  private void connect(String host, int port) throws TException {
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
   * @param name        The name of the atomic multilog to create
   * @param schema      The schema for the atomic multilog
   * @param storageMode The mode for storage
   * @throws TException Cannot create the atomic multilog
   */
  public void createAtomicMultilog(String name, Schema schema, rpc_storage_mode storageMode) throws TException {
    this.curSchema = schema;
    List<rpc_column> rpcSchema = TypeConversions.convertToRPCSchema(this.curSchema);
    curMultilogId = client.createAtomicMultilog(name, rpcSchema, storageMode);
  }

  /**
   * Creates an atomic multilog for this client
   *
   * @param name        The name of the atomic multilog to create
   * @param schema      The schema for the atomic multilog
   * @param storageMode The mode for storage
   * @throws TException Cannot create the atomic multilog
   */
  public void createAtomicMultilog(String name, String schema, rpc_storage_mode storageMode) throws TException {
    this.curSchema = new Schema(SchemaBuilder.fromString(schema));
    List<rpc_column> rpcSchema = TypeConversions.convertToRPCSchema(this.curSchema);
    curMultilogId = client.createAtomicMultilog(name, rpcSchema, storageMode);
  }

  /**
   * Load atomic multilog with given name.
   * @param name        The name of the atomic multilog
   * @throws TException Cannot load atomic multilog
   */
  public void loadAtomicMultilog(String name) throws TException {
    rpc_atomic_multilog_info info = client.loadAtomicMultilog(name);
    curSchema = TypeConversions.convertToSchema(info.getSchema());
    curMultilogId = info.getId();
  }


  /**
   * Sets the atomic multilog to the desired atomic multilog
   *
   * @param name The name of atomic multilog to set the current atomic multilog to
   * @throws TException Cannot set the atomic multilog
   */
  public void setCurrentAtomicMultilog(String name) throws TException {
    rpc_atomic_multilog_info info = client.getAtomicMultilogInfo(name);
    curSchema = TypeConversions.convertToSchema(info.getSchema());
    curMultilogId = info.getId();
  }

  /**
   * get the atomic multilog id
   *
   * @param name The name of atomic multilog to set the current atomic multilog to
   * @return  multilog id
   * @throws TException Cannot get the atomic multilog
   */
  public long getAtomicMultilogId(String name) throws TException {
    rpc_atomic_multilog_info info = client.getAtomicMultilogInfo(name);
    return info.getId();
  }

  /**
   * @return the current schema
   *
   **/
  public Schema getSchema(){
    return curSchema;
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
   * Archive atomic multilog upto provided offset.
   * @param offset Offset until which atomic multilog should be archived.
   * @throws TException Archival failure.
   */
  public void archive(long offset) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    client.archive(curMultilogId, offset);
  }

  /**
   * Archive entire atomic multilog.
   * @throws TException Archival failure.
   */
  public void archive() throws TException {
    archive(-1);
  }

  /**
   * Gets a new record batch builder
   *
   * @return The RPC record batch builder
   */
  public RecordBatchBuilder getBatchBuilder() {
    return new RecordBatchBuilder(curSchema);
  }

  /**
   * Writes a record to the atomic multilog
   *
   * @param record The record to write
   * @return The offset into the log where the record is written.
   * @throws TException Cannot append the record
   */
  public long appendRaw(ByteBuffer record) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    if (record.capacity() != curSchema.getRecordSize()) {
      throw new IllegalStateException("Record size incorrect; expected=" + curSchema.getRecordSize() + ", got="
          + record.capacity());
    }
    return client.append(curMultilogId, record);
  }

  /**
   * Writes a record to the atomic multilog
   *
   * @param record The record to write
   * @return The offset into the log where the record is written.
   * @throws TException Cannot append the record
   */
  public long append(List<String> record) throws TException {
    return appendRaw(curSchema.pack(record));
  }

  /**
   * Writes a record to the atomic multilog
   *
   * @param record The record to write
   * @return The offset into the log where the record is written.
   * @throws TException Cannot append the record
   */
  public long append(String... record) throws TException {
    return appendRaw(curSchema.pack(record));
  }

  /**
   *
   * Write a batch record to the atomic multilog
   * @param  batchRecord the batch record to write
   * @return the offset into the log where the record is written
   *
   **/
  public long appendBatch(rpc_record_batch batchRecord) throws TException{
    return client.appendBatch(curMultilogId,batchRecord);
  }

  /**
   * Reads data from a specified offset
   *
   * @param offset The offset from the log to readRaw from
   * @return The data get the offset
   * @throws TException Cannot read the record
   */
  public ByteBuffer readRaw(long offset) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    return client.read(curMultilogId, offset, 1);
  }

  /**
   * Reads data from a specified offset,
   * @param   batchSize size
   * @param offset The offset from the log to readRaw from
   * @return The data get the offset
   * @throws TException Cannot read the record
   */
  public ByteBuffer readBatchRaw(long offset,int batchSize) throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    return client.read(curMultilogId, offset, batchSize);
  }

  public Record read(long offset) throws TException {
    return curSchema.apply(readRaw(offset));
  }

  /**
   * read batch record raw and parse into record list
   *
   * @@return  record list
   **/
  public List<Record> readBatch(long offset,int batchSize) throws TException {
    ByteBuffer batchBuffer = readBatchRaw(offset, batchSize);
    List<Record> batchResult = new ArrayList<>(batchSize);
    int remaining = batchBuffer.remaining();
    ByteBuffer slice;
    for (int i = 0; i < remaining; ) {
      slice = batchBuffer.slice();
      slice.position(i);
      slice.limit(slice.position() + curSchema.getRecordSize());
      batchResult.add(curSchema.apply(slice.slice()));
      i += curSchema.getRecordSize();
    }
    return batchResult;
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

  /**
   * Gets the records size for the atomic multilog
   *
   * @return The number of records
   * @throws TException Cannot get the number of records
   */
  public long recordSize() throws TException {
    if (curMultilogId == -1) {
      throw new IllegalStateException("Must set Atomic Multilog first");
    }
    return curSchema.getRecordSize();
  }
}

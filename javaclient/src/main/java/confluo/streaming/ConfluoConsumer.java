package confluo.streaming;

import confluo.rpc.Record;
import confluo.rpc.RpcClient;
import confluo.rpc.Schema;
import confluo.streaming.config.ConsumerConfig;
import java.io.Closeable;
import java.io.IOException;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.util.List;
import java.util.Properties;

/** Confluo message consumer,thread safe */
public class ConfluoConsumer implements Closeable {
  Logger logger = LoggerFactory.getLogger(ConfluoConsumer.class);
  private String topic;
  private String host;
  private int port;
  private int prefetchSize;
  private RpcClient client;
  private boolean prefetchEnable;
  private Schema schema;

  public ConfluoConsumer(Properties properties) throws TException {

    host = properties.getProperty(ConsumerConfig.BOOTSTRAP_ADDRESS, ConsumerConfig.BOOTSTRAP_ADDRESS_DEFAULT);
    port = Integer.valueOf(properties.getProperty(ConsumerConfig.BOOTSTRAP_PORT, ConsumerConfig.BOOTSTRAP_PORT_DEFAULT));
    topic = properties.getProperty(ConsumerConfig.TOPIC, ConsumerConfig.TOPIC_DEFAULT);
    prefetchSize = Integer.valueOf(properties.getProperty(ConsumerConfig.PREFETCH_SIZE, ConsumerConfig.PREFETCH_SIZE_DEFAULT));
    prefetchEnable = prefetchSize > 1 ? true : false;
    client = new RpcClient(host, port);
    client.setCurrentAtomicMultilog(topic);
    schema = client.getSchema();
    logger.info(
        String.format(
                "\n ------------------- "
                    + "\n consumer config"
                    + "\n server address: %s,port:%d,topic:%s,prefetch size:%d",
                host, port, topic, prefetchSize)
            + "\n ------------------- ");
  }

  /**
   * best effort consume consume from target offset
   *
   * @param offset log offset
   * @return record count
   */
  public int pull(long offset, MessageListener messageListener) throws TException {
    int batchSize;
    if (prefetchEnable) {
      List<Record> records = client.readBatch(offset, prefetchSize);
      batchSize = records.size();
      messageListener.onMessage(records);
    } else {
      messageListener.onMessage(client.read(offset));
      batchSize = 1;
    }
    return batchSize;
  }

  public Schema getSchema() {
    return schema;
  }

  /** the number of records for the topic */
  public long maxRecord() throws TException {
    return client.numRecords();
  }

  @Override
  public void close() throws IOException {
    try {
      client.close();
    } catch (TException e) {
      throw new IOException(e);
    }
  }
}

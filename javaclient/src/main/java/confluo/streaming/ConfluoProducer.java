package confluo.streaming;

import confluo.rpc.*;
import confluo.streaming.config.ProducerConfig;
import java.io.Closeable;
import org.apache.thrift.TApplicationException;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Properties;

/**
 * ConfluoProducer will create the topic if it isn't existed,or reuse the old topic. batch produce
 * message can be enable if batch size was set to bigger than 1,not thread safe
 */
public class ConfluoProducer implements Closeable {
  Logger logger = LoggerFactory.getLogger(ConfluoProducer.class);
  private String topic;
  private String host;
  private int port;
  private long totalProducedNum;
  private int produceBatchSize;
  private boolean batchEnable;
  private int messageSize;
  private rpc_storage_mode storageMode;
  private String produceSchema;
  private RpcClient client;
  private MessageBatchBuilder batchBuilder;
  private Schema curSchema;
  private int batched;

  public ConfluoProducer(Properties properties) throws TException {
    host =
        properties.getProperty(
            ProducerConfig.BOOTSTRAP_ADDRESS, ProducerConfig.BOOTSTRAP_ADDRESS_DEFAULT);
    port =
        Integer.valueOf(
            properties.getProperty(
                ProducerConfig.BOOTSTRAP_PORT, ProducerConfig.BOOTSTRAP_PORT_DEFAULT));
    topic = properties.getProperty(ProducerConfig.TOPIC, ProducerConfig.TOPIC_DEFAULT);
    storageMode =
        rpc_storage_mode.valueOf(
            properties.getProperty(
                ProducerConfig.ARCHIVE_MODE, ProducerConfig.ARCHIVE_MODE_DEFAULT));
    messageSize =
        Integer.valueOf(
            properties.getProperty(
                ProducerConfig.MESSAGE_SIZE, ProducerConfig.MESSAGE_SIZE_DEFAULT));
    produceSchema = createSchema(messageSize - 8);
    produceBatchSize =
        Integer.valueOf(
            properties.getProperty(ProducerConfig.BATCH_SIZE, ProducerConfig.BATCH_SIZE_DEFAULT));
    batchEnable = produceBatchSize > 1 ? true : false;
    start();
    logger.info(
        String.format(
            "\n ------------------- "
                + "\n producer config"
                + "\n server address: %s,port:%d,topic:%s,schema:%s "
                + "\n mutlilog:%s ;batch:%s; batchSize:%d;storage mode:%s"
                + "\n ------------------- ",
            host, port, topic, produceSchema, topic, batchEnable, produceBatchSize, storageMode));
  }

  /**
   * Create a topic or reuse existed topic
   *
   * @throws TException when exception occurs
   */
  private void start() throws TException {
    client = new RpcClient(host, port);
    try {
      long atomicLogId = client.getAtomicMultilogId(topic);
      client.setCurrentAtomicMultilog(topic);
      logger.info(String.format("%s exist,atomic multilog id %d,reuse it now", topic, atomicLogId));
    } catch (TApplicationException e) {
      // process concurrently create topic
      client.createAtomicMultilog(topic, produceSchema, storageMode);
    } catch (TException e) {
      logger.info("start error", e);
      throw e;
    }
    curSchema = client.getSchema();
    if (batchEnable) {
      batchBuilder = new MessageBatchBuilder(client.getSchema());
    }
  }

  /**
   * For a pub/sub system,it's schemaless,but here we define a relatively relaxed schema{
   * long:a;msg:STRING(len)} with default timestamp,msg with fixed length is the actual payload for
   * producer
   *
   * @param len message body length
   * @return schema string
   */
  private String createSchema(int len) {
    return String.format("{msg: STRING(%d)}", len);
  }

  /** @return current schema */
  public Schema getSchema() {
    return curSchema;
  }

  /** Fill timestamp and send message, */
  public void send(ByteBuffer message) throws TException {
    try {
      message.putLong(0, System.currentTimeMillis());
      message.clear();
      if (batchEnable) {
        batchBuilder.addRecord(message);
        batched++;
        if (batched >= produceBatchSize) {
          long offset = client.appendBatch(batchBuilder.getBatch());
          // logger.info("write offset:"+offset);
          batchBuilder.clear();
          totalProducedNum += batched;
          batched = 0;
        }
      } else {
        client.appendRaw(message);
        totalProducedNum++;
      }
    } catch (IOException e) {
      logger.error("io error", e);
    }
  }

  /** @return get total produced messages */
  public long getTotalProducedNum() {
    return totalProducedNum;
  }

  /** Flush the last batch messages */
  public void flush() throws IOException, TException {
    if (batchEnable && batched > 0) {
      long offset = client.appendBatch(batchBuilder.getBatch());
      batchBuilder.clear();
      totalProducedNum += batched;
      batched = 0;
    }
  }

  /** Stop and close client */
  @Override
  public void close() throws IOException {
    try {
      flush();
      client.close();
    } catch (TException e) {
      throw new IOException(e);
    }
  }
}

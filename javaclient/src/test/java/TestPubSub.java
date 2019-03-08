import static org.junit.Assert.assertEquals;

import confluo.PropertiesParser;
import confluo.rpc.Record;
import confluo.rpc.Schema;
import confluo.streaming.ConfluoConsumer;
import confluo.streaming.ConfluoProducer;
import confluo.streaming.ConsumeTask;
import confluo.streaming.ProduceTask;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.Random;
import java.util.function.Consumer;
import org.apache.thrift.TException;
import org.junit.Test;

public class TestPubSub {
  private static final char[] charTable =
    new char[] {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
      'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z','a', 'b', 'c', 'e', 'e', 'f', 'g', 'h', 'i', 'j',
      'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };
  
  @Test
  public void testConfluoPubSub()throws IOException, TException {
    Properties producerProperties = PropertiesParser.parse("mq.produce", "mq.properties");
    Properties consumerProperties = PropertiesParser.parse("mq.consume", "mq.properties");
    ConfluoProducer producer=new ConfluoProducer(producerProperties);
    ConfluoConsumer consumer=new ConfluoConsumer(consumerProperties);
    long startOffset=consumer.maxRecord();
    Schema schema=producer.getSchema();
    int recordSize=consumer.getSchema().getRecordSize();
    ByteBuffer messageA=randomMessage(producer.getSchema());
    ByteBuffer messageB=randomMessage(producer.getSchema());
    producer.send(messageA);messageA.clear();
    producer.send(messageB);messageB.clear();
    List<Record> orderSendMessages=new ArrayList();
    List<Record> orderAcceptedMessages=new ArrayList();
    orderSendMessages.add(schema.apply(messageA));
    orderSendMessages.add(schema.apply(messageB));
    producer.close();
    while (orderAcceptedMessages.size() < orderSendMessages.size()) {
      consumer.pull(startOffset * recordSize,new Consumer<List<Record>>() {
            @Override
            public void accept(List<Record> records) {
              orderAcceptedMessages.addAll(records);
            }
          });
    }
    for (int i = 0; i < orderSendMessages.size(); i++) {
      assertEquals(orderSendMessages.get(i).toString(),orderAcceptedMessages.get(i).toString());
    }
    consumer.close();
  }

  /**
   * @param schema message schema
   * @return byte buffer message
   */
  public ByteBuffer randomMessage(Schema schema){
    Random rd = new Random();
    ByteBuffer buffer = ByteBuffer.allocate(schema.getRecordSize());
    buffer.order(ByteOrder.LITTLE_ENDIAN);
    byte b;
    buffer.putLong(System.currentTimeMillis());
    for (int i = 8; i < schema.getRecordSize(); i++) {
      b = (byte) charTable[rd.nextInt(62)];
      buffer.put(b);
    }
    return buffer;
  }
}

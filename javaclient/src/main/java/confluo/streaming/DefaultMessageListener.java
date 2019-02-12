package confluo.streaming;

import confluo.rpc.Record;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;

public class DefaultMessageListener implements MessageListener {
  Logger logger = LoggerFactory.getLogger(DefaultMessageListener.class);
  private long receive = 0;
  private long logSample;

  public DefaultMessageListener(long logSample) {
    this.logSample = logSample;
  }

  @Override
  public void onMessage(List<Record> messages) {
    receive += messages.size();
    if (receive % logSample == 0) {
      log(messages.get(0));
    }
  }

  @Override
  public void onMessage(Record message) {
    receive++;
    if (receive % logSample == 0) {
      log(message);
    }
  }

  public void log(Record record) {
    if (record != null)
      this.logger.info(
          String.format(
              "timestamp:%d,message:%s", record.get(0).asLong(), record.get(1).asString()));
  }
}

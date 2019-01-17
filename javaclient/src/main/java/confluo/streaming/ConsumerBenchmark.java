package confluo.streaming;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Properties;

public class ConsumerBenchmark {
    public static void main(String[] args) {
        Logger logger = LoggerFactory.getLogger(ConsumerBenchmark.class);
        Properties properties=new Properties();
        URL propertiesUrl=ConfluoConsumer.class.getClassLoader().getResource("mq.properties");
        try {
            InputStream in = propertiesUrl.openStream();
            properties.load(in);
        }catch (IOException e){
            logger.info("io error",e);
        }
        int consumeNum=Integer.valueOf(properties.getProperty("mq.consume.concurrency","10"));
        long durationMs=Long.valueOf(properties.getProperty("mq.consume.duration","60000"));
        long consumeLogSampleMs=Long.valueOf(properties.getProperty("mq.consume.log.sample","10000"));
        long consumeStartOffset=Long.valueOf(properties.getProperty("mq.consume.message.min.index","0"));
        logger.info(String.format("%d consumer and duration %d ms",consumeNum,durationMs));
        Thread[] consumers=new Thread[consumeNum];
        ConsumeTask[] consumerTasks=new ConsumeTask[consumeNum];
        for(int i=0;i<consumeNum;i++){
            consumerTasks[i]=new ConsumeTask(durationMs,consumeLogSampleMs,consumeStartOffset);
            consumers[i]=new Thread(consumerTasks[i],String.valueOf(i));
            consumers[i].start();
        }
        try {
            for (Thread t : consumers) {
                t.join();
            }
            long totalConsumedMsgNum=0;
            for(ConsumeTask task:consumerTasks){
                totalConsumedMsgNum+=task.getConsumedMessage();
            }
            long qps=totalConsumedMsgNum *1000/durationMs;
            logger.error(String.format("%d consume finished,total msg:%d, elapsed:%d ms, qps:%d/s",consumeNum,totalConsumedMsgNum,durationMs,qps));
        }catch (InterruptedException e){
            logger.info("interrupted",e);
        }
    }
}

package confluo.streaming.benchmark;

import confluo.PropertiesParser;
import confluo.streaming.ConsumeTask;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.io.IOException;
import java.util.Properties;

public class ConsumerBenchmark {
    public static void main(String[] args) {
        Logger logger = LoggerFactory.getLogger(ConsumerBenchmark.class);
        Properties properties;
        try {
            properties = PropertiesParser.parse("mq.consume.benchmark","mq.properties");
        }catch (IOException e) {
            logger.info("parse properties error", e);
            throw new IllegalStateException("init exception", e);
        }
        int  consumeNum=Integer.valueOf(properties.getProperty("concurrency","10"));
        long durationMs=Long.valueOf(properties.getProperty("duration","60000"));
        long consumeLogSampleMs=Long.valueOf(properties.getProperty("log.sample","10000"));
        long consumeStartOffset=Long.valueOf(properties.getProperty("message.min.index","0"));
        logger.info(String.format("%d consumer and duration %d ms",consumeNum,durationMs));
        Thread[] consumers=new Thread[consumeNum];
        ConsumeTask[] consumerTasks=new ConsumeTask[consumeNum];
        long start=System.currentTimeMillis();
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
            long acutalDurationMs=System.currentTimeMillis()-start;
            long qps=totalConsumedMsgNum *1000/acutalDurationMs;
            logger.error(String.format("%d consumer finished,total msg:%d, elapsed:%d ms, qps:%d/s",consumeNum,totalConsumedMsgNum,acutalDurationMs,qps));
        }catch (InterruptedException e){
            logger.info("interrupted",e);
        }
    }
}

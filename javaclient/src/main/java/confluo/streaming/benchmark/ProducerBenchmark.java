package confluo.streaming.benchmark;

import confluo.PropertiesParser;
import confluo.streaming.ProduceTask;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.io.IOException;
import java.util.Properties;

public class ProducerBenchmark {
    public static void main(String[] args) {
        Logger logger = LoggerFactory.getLogger(ProducerBenchmark.class);
        Properties properties;
        try {
            properties = PropertiesParser.parse("mq.produce.benchmark","mq.properties");
        }catch (IOException e) {
            logger.info("parse properties error", e);
            throw new IllegalStateException("init exception", e);
        }
        int producerNum=Integer.valueOf(properties.getProperty("concurrency","10"));
        int durationMs=Integer.valueOf(properties.getProperty("duration","60000"));
        int logSampleMs=Integer.valueOf(properties.getProperty("log.sample","30000"));
        logger.info(String.format("%d producer and duration %d ms",producerNum,durationMs));
        long startTime=System.currentTimeMillis();
         Thread[] producers=new Thread[producerNum];
         ProduceTask[] producerTasks=new ProduceTask[producerNum];
         for(int i=0;i<producerNum;i++){
             producerTasks[i]=new ProduceTask(durationMs,logSampleMs);
             producers[i]=new Thread(producerTasks[i],String.valueOf(i));
             producers[i].start();
         }
         try {
             for (Thread t : producers) {
                 t.join();
             }
             long totalProducedMsgNum=0;
             for(ProduceTask task:producerTasks){
                 totalProducedMsgNum+=task.getProducedMessage();
             }
             long actualDurationMs=System.currentTimeMillis()-startTime;
             long qps=totalProducedMsgNum *1000/actualDurationMs;
             logger.info(String.format("%d producer finished,total msg:%d, elapsed:%d ms, qps:%d/s",producerNum,totalProducedMsgNum,actualDurationMs,qps));
         }catch (InterruptedException e){
             logger.info("interrupted",e);
         }
    }
}

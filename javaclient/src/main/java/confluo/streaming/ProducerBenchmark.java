package confluo.streaming;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Properties;

public class ProducerBenchmark {
    public static void main(String[] args) {
        Logger logger = LoggerFactory.getLogger(ProducerBenchmark.class);
        Properties properties=new Properties();
        URL propertiesUrl=ConfluoConsumer.class.getClassLoader().getResource("mq.properties");
        try {
            InputStream in = propertiesUrl.openStream();
            properties.load(in);
        }catch (IOException e){
            logger.info("io error",e);
        }
        int producerNum=Integer.valueOf(properties.getProperty("mq.produce.concurrency","10"));
        int durationMs=Integer.valueOf(properties.getProperty("mq.produce.duration","60000"));
        logger.info(String.format("%d producer and duration %d ms",producerNum,durationMs));
        long startTime=System.currentTimeMillis();
         Thread[] producers=new Thread[producerNum];
         ProduceTask[] producerTasks=new ProduceTask[producerNum];
         for(int i=0;i<producerNum;i++){
             producerTasks[i]=new ProduceTask(durationMs);
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

package confluo.streaming;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ProducerBenchmark {
    public static void main(String[] args) {
        Logger logger = LoggerFactory.getLogger(ProducerBenchmark.class);
         int producerNum=10;
         long durationMs=5*60*1000;
         Thread[] producers=new Thread[producerNum];
         for(int i=0;i<producerNum;i++){
             producers[i]=new Thread(new ProducerTask(durationMs));
         }
         try {
             for (Thread t : producers) {
                 t.start();
                 t.join();
             }
         }catch (InterruptedException e){
             logger.info("interrupted",e);
         }
    }
}

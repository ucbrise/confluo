package confluo.streaming;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ProducerTask implements Runnable {
    Logger logger = LoggerFactory.getLogger(ProducerTask.class);
    ConfluoProducer producer = new ConfluoProducer();
    private long durationMs;
    private long start;
    public ProducerTask(long durationMs){
        this.durationMs=durationMs;

    }
    @Override
    public void run() {
        //long maxProduceTime=60*1000;
        try {
            logger.error("start to produce");
            producer.start();
            start=System.currentTimeMillis();
            while(true){
                producer.produce();
                long time= System.currentTimeMillis()-start;
                if(time>durationMs){
                    break;
                }
            }
            producer.flush();
            producer.stop();
            long time=System.currentTimeMillis()-start;
            long qps=producer.getTotalProduceNum() *1000/time;
            logger.error(String.format("produce end,total msg:%d, elapsed:%d ms, qps:%d/s",producer.getTotalProduceNum(),time,qps));
        } catch (Exception e) {
            logger.info("error", e);
        }
    }
}

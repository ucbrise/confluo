package confluo.streaming;

import confluo.rpc.Schema;
import confluo.rpc.rpc_management_exception;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.nio.ByteBuffer;


/**
 * timed producer task
 *
 **/
public class ProduceTask implements Runnable {
    Logger logger = LoggerFactory.getLogger(ProduceTask.class);
    ConfluoProducer producer = new ConfluoProducer();
    private long durationMs;
    private long start;
    private long sampleMs=30*1000;
    public ProduceTask(long durationMs){
        this.durationMs=durationMs;

    }
    public long getProducedMessage(){
        return producer.getTotalProduceNum();
    }
    @Override
    public void run() {
        //long maxProduceTime=60*1000;
        long logBreakMs=1000;
        long lastLogTimeMs=0;
        try {
            logger.error("start to produce");
            int maxTry=3;
            int tryCount=0;
            while(tryCount++<=maxTry) { //concurrent create exception
                try {
                    producer.start();
                    logger.info(String.format("start finished, tried %d time",tryCount));
                    break;
                } catch (rpc_management_exception e) {
                    logger.info("start error",e);
                }
            }

            Schema curSchema=producer.getSchema();
            // prepare a message
            ByteBuffer message=producer.byteMessage(curSchema.getRecordSize());
            start=System.currentTimeMillis();
            while(true){
                producer.produce(message);
                long time= System.currentTimeMillis()-start;
                // may log multiple times in the same ms
                if(time%sampleMs==0&&(time-lastLogTimeMs)>logBreakMs){
                    lastLogTimeMs=time;
                    logger.info(String.format("time elapsed %d(ms),%s produced  %d message",time,Thread.currentThread().getName(),producer.getTotalProduceNum()));
                }
                if(time>durationMs){
                    break;
                }
            }
            producer.flush();
            producer.stop();
            long time=System.currentTimeMillis()-start;
            long qps=producer.getTotalProduceNum() *1000/time;
            logger.info(String.format("produce end,total msg:%d, elapsed:%d ms, qps:%d/s",producer.getTotalProduceNum(),time,qps));
        } catch (Exception e) {
            logger.info("error", e);
        }
    }
}

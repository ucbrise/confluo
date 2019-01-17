package confluo.streaming;

import confluo.rpc.Schema;
import confluo.rpc.rpc_management_exception;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
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
            logger.info("start to produce");
            int maxTry=3;
            int tryCount=0;
            boolean startSuccessful=false;
            while(tryCount++<maxTry) { //concurrent create exception
                try {
                    producer.start();
                    logger.info(String.format("start finished, tried %d time",tryCount));
                    startSuccessful=true;
                    break;
                } catch (rpc_management_exception e) {
                    logger.info("start error",e);
                }
            }

            if(!startSuccessful){ logger.info("start failure");stop();return;}
            Schema curSchema=producer.getSchema();
            // prepare a default message, random generate
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
            stop();
            long time=System.currentTimeMillis()-start;
            long qps=producer.getTotalProduceNum() *1000/time;
            logger.info(String.format("time elapsed:%d ms,%s produce end,total msg:%d,  qps:%d/s",time,Thread.currentThread().getName(),producer.getTotalProduceNum(),qps));
        } catch (Exception e) {
            logger.info("error", e);
        }
    }
    public void stop() throws TException, IOException {
        producer.flush();
        producer.stop();
    }
}

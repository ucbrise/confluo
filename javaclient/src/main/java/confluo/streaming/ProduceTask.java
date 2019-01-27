package confluo.streaming;

import confluo.PropertiesParser;
import confluo.rpc.Schema;
import confluo.rpc.rpc_management_exception;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Properties;
import java.util.Random;


/**
 * timed producer task
 *
 **/
public class ProduceTask implements Runnable {
    Logger logger = LoggerFactory.getLogger(ProduceTask.class);
    private static final char[] encodeTable = new char[]{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
            'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6',
            '7', '8', '9'};
    Properties properties;
    // init
    ConfluoProducer producer;
    private long durationMs;
    private long start;
    private long sampleMs;
    public ProduceTask(long durationMs,long produceLogSample){
        this.durationMs=durationMs;
        this.sampleMs=produceLogSample;
        try {
            properties = PropertiesParser.parse("mq.produce", "mq.properties");
        }catch (IOException e){
            logger.info("parse properties error",e);
            throw new IllegalStateException("init exception",e);
        }
             producer= new ConfluoProducer(properties);
    }
    public long getProducedMessage(){
        return producer.getTotalProducedNum();
    }
    @Override
    public void run() {
        //long maxProduceTime=60*1000;
        long logBreakMs=1000;
        long lastLogTimeMs=0;
        try {
            logger.info("start to send");
            int maxTry=3;
            int tryCount=0;
            boolean startSuccessful=false;
            while(tryCount++<maxTry) { //concurrent create exception
                try {
                    producer.start();
                    logger.info(String.format("start finished, tried %d times ",tryCount));
                    startSuccessful=true;
                    break;
                } catch (rpc_management_exception e) {
                    logger.info("start error",e);
                }
            }

            if(!startSuccessful){ logger.info("start failure");stop();return;}
            Schema curSchema=producer.getSchema();
            // prepare a default message, random generate
            ByteBuffer message=byteMessage(curSchema.getRecordSize());
            start=System.currentTimeMillis();
            while(true){
                producer.send(message);
                long time= System.currentTimeMillis()-start;
                // may log multiple times in the same ms
                if(time%sampleMs==0&&(time-lastLogTimeMs)>logBreakMs){
                    lastLogTimeMs=time;
                    logger.info(String.format("time elapsed %d(ms),%s produced  %d message",time,Thread.currentThread().getName(),producer.getTotalProducedNum()));
                }
                if(time>durationMs){
                    break;
                }
            }
            stop();
            long time=System.currentTimeMillis()-start;
            long qps=producer.getTotalProducedNum() *1000/time;
            logger.info(String.format("time elapsed:%d ms,%s send end,total msg:%d,  qps:%d/s",time,Thread.currentThread().getName(),producer.getTotalProducedNum(),qps));
        } catch (Exception e) {
            logger.info("error", e);
        }
    }

    /**
     * @param len message length
     * @return    byte buffer message
     *
     **/
    public ByteBuffer byteMessage(int len){
        Random rd = new Random();
        ByteBuffer buffer=ByteBuffer.allocate(len);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        byte b;
        // skip 8 byte for timestamp
        buffer.position(8);
        for (int i =8; i < len; i++) {
            b=(byte) encodeTable[rd.nextInt(36)];
            buffer.put(b);
        }
        return  buffer;

    }
    public void stop() throws TException, IOException {
        producer.flush();
        producer.stop();
    }
}

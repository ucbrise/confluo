package confluo.streaming;

import confluo.PropertiesParser;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.Properties;


/**
 *  best effort consume before timeout
 *
 **/
public class ConsumeTask implements Runnable {
    Logger logger= LoggerFactory.getLogger(ConsumeTask.class);
    private long consumeLogSample;
    private long consumeOffset;
    private long totalRead=0;
    private long maxConsumeMs;
    private Properties properties;
    ConfluoConsumer consumer;
    public ConsumeTask(long maxConsumeMs,long consumeLogSample,long consumeOffset){
        this.consumeLogSample=consumeLogSample;
        this.consumeOffset=consumeOffset;
        this.maxConsumeMs=maxConsumeMs;
        try {
            properties = PropertiesParser.parse("mq.consume", "mq.properties");
        }catch (IOException e){
            logger.info("parse properties error",e);
            throw new IllegalStateException("init exception",e);
        }

    }

    /**
     *  @return  total consumed messages
     **/
    public long getConsumedMessage(){
        return totalRead;
    }
    @Override
    public void run() {
        long consumeMaxTime=maxConsumeMs;
        long read=0;
        MessageListener messageListener=new DefaultMessageListener(consumeLogSample);
        try {
            consumer=new ConfluoConsumer(properties);
            int  recordSize=consumer.getSchema().getRecordSize();
            long start=System.currentTimeMillis();
            long elapsed=1; //
            int startOffsetEqualMaxCount=0;
            while(true) {
                long startOffset = Math.max(consumeOffset, 0) * recordSize;// real offset in log
                long maxOffset = consumer.maxRecord() * recordSize;
                if(startOffset==maxOffset){
                    //logger.info("");
                    // no more new message,consider exit
                    startOffsetEqualMaxCount++;
                    if(startOffsetEqualMaxCount>10){
                        logger.info("possible no more messages");
                        break;
                    }
                    Thread.sleep(1);
                }
                long count;
                long i= startOffset;
                for ( ; i < maxOffset; ) {
                    count = consumer.pull(i, messageListener);
                    totalRead += count;
                    read += count;
                    i += recordSize * count;
                    if(totalRead%10000==0&&(System.currentTimeMillis() - start)>consumeMaxTime){
                        logger.info("timeout and exit");
                        break;
                    }
                }
                consumeOffset +=read;
                read = 0;
                elapsed = System.currentTimeMillis() - start;
                if (elapsed >= consumeMaxTime) {
                    break;
                }
            }
            long qps=totalRead *1000/elapsed;
            logger.info(String.format("total msg:%d, elapsed:%d ms, qps:%d/s",totalRead,elapsed,qps));
        }catch (TException e){
            logger.info("error",e);
        }catch (InterruptedException e){
            logger.info("interrupted");
        }
    }
}

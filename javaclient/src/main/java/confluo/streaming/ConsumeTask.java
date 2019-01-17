package confluo.streaming;

import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ConsumeTask implements Runnable {
    Logger logger= LoggerFactory.getLogger(ConsumeTask.class);
    private long consumeLogSample;
    private long recordOffset;
    private long totalRead=0;
    private long maxConsumeMs;
    ConfluoConsumer consumer=new ConfluoConsumer();
    public ConsumeTask(long maxConsumeMs,long consumeLogSample,long recordOffset){
        this.consumeLogSample=consumeLogSample;
        this.recordOffset=recordOffset;
        this.maxConsumeMs=maxConsumeMs;
    }

    /**
     *  返回消费的总消息数
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
            consumer.start();
            int  recordSize=consumer.getSchema().getRecordSize();
            long start=System.currentTimeMillis();
            long time;
            while(true) {
                long startOffset = Math.max(recordOffset, 0) * recordSize;// real offset in log
                long maxOffset = consumer.maxRecord() * recordSize;
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
                recordOffset +=read;
                read = 0;
                time = System.currentTimeMillis() - start;
                if (time >= consumeMaxTime) {
                    break;
                }
            }
            long qps=totalRead *1000/time;
            logger.info(String.format("total msg:%d, elapsed:%d ms, qps:%d/s",totalRead,time,qps));
        }catch (TException e){
            logger.info("error",e);
        }
    }
}

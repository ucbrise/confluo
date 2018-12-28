package confluo.streaming;

import confluo.rpc.Record;
import confluo.rpc.RpcClient;
import confluo.rpc.Schema;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.List;
import java.util.Properties;

public class ConfluoConsumer {
    Logger logger= LoggerFactory.getLogger(ConfluoConsumer.class);
    private Properties properties=new Properties();
    private String topic;
    private String host;
    private int port;

    private int  prefetchSize;
    private  RpcClient client;
    private boolean prefetchEnable;
    private int consumeLogSample;
    private int totalRead =0;
    private Schema schema;
    public ConfluoConsumer(){
        URL propertiesUrl=ConfluoConsumer.class.getClassLoader().getResource("mq.properties");
        try {
            InputStream in  = propertiesUrl.openStream();
            properties.load(in);
            topic=properties.getProperty("mq.consume.topic");
            host=properties.getProperty("mq.server.address");
            port=Integer.valueOf(properties.getProperty("mq.server.port"));
            prefetchSize=Integer.valueOf(properties.getProperty("mq.consume.prefetch.size"));
            prefetchEnable=Boolean.valueOf(properties.getProperty("mq.consume.prefetch.enable","false"));
            logger.info(String.format("server address: %s,port:%d,topic:%s",host,port,topic));
        }catch (IOException e){
            logger.info("io error",e);
        }
    }

    /**
     *
     **/
    public void start() throws TException {
        client= new RpcClient(host, port);
        client.setCurrentAtomicMultilog(topic);
        schema=client.getSchema();
    }

    /**
     * best effort consume
     * consume from target offset
     * @param offset long offset
     * @return  result count
     **/
    public int pull(long offset,MessageListener messageListener) throws TException{
        int  batchSize;
            if(prefetchEnable) {
                List<Record> records = client.readBatch(offset, prefetchSize);
                batchSize=records.size();
                messageListener.onMessage(records);
            }else{
                messageListener.onMessage(client.read(offset));
                batchSize=1;
            }
            return  batchSize;
    }

    public Schema getSchema(){
        return schema;
    }

    /**
     * 消息条数
     **/
    public long maxRecord() throws TException{
        return client.numRecords();
    }

    public static void main(String[] args){
        Logger logger= LoggerFactory.getLogger(ConfluoConsumer.class);
        Properties properties=new Properties();
        URL propertiesUrl=ConfluoConsumer.class.getClassLoader().getResource("mq.properties");
        try {
            InputStream in = propertiesUrl.openStream();
            properties.load(in);
        }catch (IOException e){
            logger.info("io error",e);
        }
        ConfluoConsumer consumer=new ConfluoConsumer();
        long consumeMaxTime=1*1000;
        long start=System.currentTimeMillis();
        long totalRead=0;
        long read=0;
        long time;
        long consumeLogSample=Integer.valueOf(properties.getProperty("mq.consume.log.sample","1"));
        MessageListener messageListener=new DefaultMessageListener(consumeLogSample);
        try {
            long minIndex=Long.valueOf(properties.getProperty("mq.consume.message.min.index","0"));
            consumer.start();
            int  recordSize=consumer.getSchema().getRecordSize();
            while(true) {
                long startOffset = Math.max(minIndex, 0) * recordSize;// real offset in log
                long maxOffset = consumer.maxRecord() * recordSize;
                long count;
                long i= startOffset;
                for ( ; i < maxOffset; ) {
                    count = consumer.pull(i, messageListener);
                    totalRead += count;
                    read += count;
                    i += recordSize * count;
                }
                minIndex +=read;
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

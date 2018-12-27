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
import java.util.Properties;

public class ConfluoConsumer {
    Logger logger= LoggerFactory.getLogger(ConfluoConsumer.class);
    private Properties properties=new Properties();
    private String topic;
    private String host;
    private int port;
    private long minIndex;
    private long maxIndex;
    private  RpcClient client;
    private int read;
    public ConfluoConsumer(){
        URL propertiesUrl=ConfluoConsumer.class.getClassLoader().getResource("mq.properties");
        try {
            InputStream in  = propertiesUrl.openStream();
            properties.load(in);
            topic=properties.getProperty("mq.consume.topic");
            host=properties.getProperty("mq.server.address");
            port=Integer.valueOf(properties.getProperty("mq.server.port"));
            minIndex=Long.valueOf(properties.getProperty("mq.consume.message.min.index"));
            maxIndex=Long.valueOf(properties.getProperty("mq.consume.message.max.index"));
            logger.info(String.format("server address: %s,port:%d,topic:%s",host,port,topic));
        }catch (IOException e){

        }
    }

    /**
     *
     **/
    public void start() throws TException {
        client= new RpcClient(host, port);
        client.setCurrentAtomicMultilog(topic);
    }

    /**
     * consume from target offset
     *
     * @param offset  record index offset
     *
     **/
    public void consume(long offset) throws TException{
        Record record;
        Schema schema=client.getSchema();
        if(schema==null){
            throw new IllegalStateException("schema is null");
        }
        int  recordSize=schema.getRecordSize();
        long startOffset=Math.max(minIndex,offset)*recordSize;// real offset in log
        long maxOffset= Math.min(client.numRecords(),maxIndex)*recordSize;
        long logMod=1000;
        for(long i=startOffset;i<maxOffset;){
             record = client.read(i);
             if(record!=null){
                 if(i%logMod==0){
                     logger.info(String.format("offset:%d,timestamp:%d,message:%s",i,record.get(0).asLong(),record.get(1).asString()));
                 }
             }
             read++;
             i+=recordSize;
        }

    }

    public static void main(String[] args){
        Logger logger= LoggerFactory.getLogger(ConfluoConsumer.class);
        ConfluoConsumer consumer=new ConfluoConsumer();
        try {
            long start=System.currentTimeMillis();
            consumer.start();
            consumer.consume(0);
            long time=System.currentTimeMillis()-start;
            long qps=consumer.read*1000/time;
            logger.info(String.format("total msg:%d, elapsed:%d ms, qps:%d/s",consumer.read,time,qps));
        }catch (TException e){
            logger.info("error",e);
        }
    }
}

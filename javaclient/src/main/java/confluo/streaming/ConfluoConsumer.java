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

}

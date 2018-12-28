package confluo.streaming;

import confluo.rpc.MessageBatchBuilder;
import confluo.rpc.RpcClient;
import confluo.rpc.Schema;
import confluo.rpc.rpc_storage_mode;
import org.apache.thrift.TApplicationException;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Properties;
import java.util.Random;

public class ConfluoProducer {
    Logger logger= LoggerFactory.getLogger(ConfluoProducer.class);
    private static final char[] encodeTable = new char[]{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
            'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6',
            '7', '8', '9'};
    private Properties properties=new Properties();
    private String topic;
    private String host;
    private int port;
    private long produceMaxNum;
    private int produceBathSize;
    private boolean batchEnable=false;
    private int messageSize;
    private rpc_storage_mode storageMode;
    private String produceSchema;
    private ByteBuffer message;
    private RpcClient client;
    public ConfluoProducer(){
        URL propertiesUrl=ConfluoProducer.class.getClassLoader().getResource("mq.properties");
        try {
            InputStream in  = propertiesUrl.openStream();
            properties.load(in);
            host=properties.getProperty("mq.server.address");
            port=Integer.valueOf(properties.getProperty("mq.server.port"));
            topic=properties.getProperty("mq.produce.topic");
            produceMaxNum =Integer.valueOf(properties.getProperty("mq.produce.message.maxNum"));
            storageMode=rpc_storage_mode.valueOf(properties.getProperty("mq.produce.archive.mode"));
            messageSize=Integer.valueOf(properties.getProperty("mq.produce.message.size","64"));
            produceSchema=getSchema(messageSize-8);
            batchEnable=Boolean.valueOf(properties.getProperty("mq.produce.batch.enable","false"));
            produceBathSize=Integer.valueOf(properties.getProperty("mq.produce.batch.size","20"));
            message=random(messageSize);
            logger.info(String.format("\n server address: %s,port:%d,topic:%s,schema:%s \n mutlilog:%s ;batch:%s; batchSize:%d;storage mode:%s",
                     host,port,topic,produceSchema,topic,batchEnable,produceBathSize,storageMode));

        }catch (IOException e){
            logger.info("io error",e);
        }
    }

    /**
     *  create a topic or reuse existed topic
     *
     **/
    public void start() throws TException {
        client= new RpcClient(host, port);
        try {
           long atomicLogId= client.getAtomicMultilog(topic);
            if(atomicLogId>0){
                // exist
                client.setCurrentAtomicMultilog(topic);
                logger.info(String.format("%s exist,reuse it now",topic));
            }
        }catch (TApplicationException e){
               client.createAtomicMultilog(topic,produceSchema,storageMode);
        }
    }

    /**
     *  default a timestamp
     *  actual  schema
     *  { long:a;msg:STRING(len)}
     *
     **/
    public String getSchema(int len){
      return String.format("{ msg: STRING(%d)}",len);
     }


     /**
      * @return  random  byte buffer
      **/
    public ByteBuffer random(int len){
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

    /**
     * produce
     * @param batch  true to enable
     **/
    public void produce(boolean batch) throws TException{
        //long logMod=100;
        Schema schema=client.getSchema();
        MessageBatchBuilder batchBuilder=new MessageBatchBuilder(schema);
        int batched=0;
        try {
            for (long i = 0; i < produceMaxNum; i++) {
                // bug  start with small num
                // message.putLong(8,i);
                message.putLong(0, System.currentTimeMillis());
                message.clear();
                if(batch) {
                    batchBuilder.addRecord(message);
                    batched++;
                    if(batched>=produceBathSize){
                        long offset=client.appendBatch(batchBuilder.getBatch());
                        //logger.info("write offset:"+offset);
                        batchBuilder.clear();
                        batched=0;
                    }
                }else {
                    client.appendRaw(message);
                }
            }
            if(batched>0){
                client.appendBatch(batchBuilder.getBatch());
                logger.info(String.format("final send batch size %d",batched));
                batchBuilder.clear();
            }
        }catch (IOException e){
            logger.info("io error",e);
        }
    }

    public void stop() throws  TException{
        client.disconnect();
        client.close();
    }

    public static void main(String[] args) {
        Logger logger = LoggerFactory.getLogger(ConfluoProducer.class);
        ConfluoProducer producer = new ConfluoProducer();
        try {
            logger.info("start to produce");
            long start=System.currentTimeMillis();
            producer.start();
            producer.produce(false);
            producer.stop();
            long time=System.currentTimeMillis()-start;
            long qps=producer.produceMaxNum *1000/time;
            logger.info(String.format("produce end,total msg:%d, elapsed:%d ms, qps:%d/s",producer.produceMaxNum,time,qps));
        } catch (TException e) {
            logger.info("error", e);
        }
    }


}

package confluo.streaming;

import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ProducerTest implements Runnable {
    private final static Logger logger= LoggerFactory.getLogger(ProducerTest.class);

    private static long start;
//    static{
//        try{
//            logger.error("start to produce");
//            producer.start();
//            start=System.currentTimeMillis();
//        }catch (TException e){
//            logger.info("te" ,e);
//        }
//
//    }

    public   void run() {
        ThreadLocal<ConfluoProducer> producerThreadLocal = new ThreadLocal<>();
        try {
           ConfluoProducer producer= producerThreadLocal.get();
           if(producer==null){
                   producer=new ConfluoProducer();
                   producer.start();
                   producerThreadLocal.set(producer);
           }
            int i=100000;
            while(i-->0)
                producer.produce();
        } catch (TException e) {
            logger.error("error", e);
        }finally {

        }
    }

   public static void main(String[] args){
        ProducerTest producerTest=new ProducerTest();
        for(int i=0;i<4;i++){
          Thread  t=new Thread(producerTest);
          t.start();
        }
   }

}

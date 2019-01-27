import confluo.streaming.ConsumeTask;
import confluo.streaming.ProduceTask;
import org.junit.Test;

public class TestPubSub {


    @Test
    public void publish(){
        ProduceTask produceTask=new ProduceTask(60000,20000);
        produceTask.run();
    }

    @Test
    public void consume(){
        ConsumeTask consumeTask=new ConsumeTask(60000,10000,0);
        consumeTask.run();
    }
}

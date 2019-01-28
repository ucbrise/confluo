import confluo.streaming.ConsumeTask;
import confluo.streaming.ProduceTask;
import org.junit.Test;

public class TestPubSub {


    @Test
    public void produce(){
        ProduceTask produceTask=new ProduceTask(10000,5000);
        produceTask.run();
    }

    @Test
    public void consume(){
        ConsumeTask consumeTask=new ConsumeTask(10000,10000,0);
        consumeTask.run();
    }
}

package confluo.streaming;

import confluo.rpc.Record;

import java.util.List;

public interface MessageListener {
    void onMessage(List<Record> messages);
    void onMessage(Record message);
}

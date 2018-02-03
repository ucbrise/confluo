package confluo.rpc;

import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TTransport;
import org.apache.thrift.transport.TSocket;

import java.nio.ByteBuffer;
import java.util.List;

public class RPCClient {

    private TSocket socket;
    private TTransport transport;
    private TBinaryProtocol protocol;
    private rpc_service.Client client;
    private long curMultilogId;
    private rpc_atomic_multilog_info info;
    private Schema curSchema;

    public RPCClient(String host, int port) {
        connect(host, port);
        curMultilogId = -1;
    }

    public void close() {
        disconnect();
    }

    public void connect(String host, int port) {
        socket = new TSocket(host, port);
        //transport = new TSocket(host, port);//TFramedTransport(socket);
        transport = socket;
        protocol = new TBinaryProtocol(transport);
        client = new rpc_service.Client(protocol);
        try {
            transport.open();
            client.register_handler();
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void disconnect() {
        if (transport.isOpen()) {
            try {
                client.deregister_handler();
            } catch (TException e) {
                e.printStackTrace();
            }
            transport.close();
        }
    }

    public void create_atomic_multilog(String atomicMultilogName, Schema schema, rpc_storage_mode storageMode) {
        this.curSchema = schema;
        List<rpc_column> rpcSchema = RPCTypeConversions.convertToRPCSchema(schema);
        try {
            curMultilogId = client.create_atomic_multilog(atomicMultilogName, rpcSchema, storageMode);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void setCurrentAtomicMultilog(String atomicMultilogName) {
        try {
            info = client.get_atomic_multilog_info(atomicMultilogName);
        } catch (TException e) {
            e.printStackTrace();
        }
        curSchema = RPCTypeConversions.convertToSchema(info.get_schema());
        curMultilogId = info.get_id();
    }

    public void removeAtomicMultilog() {
        if (curMultilogId == -1) {

        }
        try {
            client.remove_atomic_multilog(curMultilogId);
            curMultilogId = -1;
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void add_index(String fieldName, double bucketSize) {
        if (curMultilogId == -1) {

        }
        try {
            client.add_index(curMultilogId, fieldName, bucketSize);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void removeIndex(String fieldName) {
        if (curMultilogId == -1) {

        }
        try {
            client.remove_index(curMultilogId, fieldName);
        } catch (TException e) {
            e.printStackTrace();
        }
    }


    public void add_filter(String filterName, String filterExpr) {
        if (curMultilogId == -1) {

        }
        try {
            client.add_filter(curMultilogId, filterName, filterExpr);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void removeFilter(String filterName) {
        if (curMultilogId == -1) {

        }
        try {
            client.remove_filter(curMultilogId, filterName);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void add_aggregate(String aggregateName, String filterName, String aggregateExpr) {
        if (curMultilogId == -1) {

        }
        try {
            client.add_aggregate(curMultilogId, aggregateName, filterName, aggregateExpr);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void removeAggregate(String aggregateName) {
        if (curMultilogId == -1) {

        }
        try {
            client.remove_aggregate(curMultilogId, aggregateName);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void install_trigger(String triggerName, String triggerExpr) {
        if (curMultilogId == -1) {

        }
        try {
            client.add_trigger(curMultilogId, triggerName, triggerExpr);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void removeTrigger(String triggerName) {
        if (curMultilogId == -1) {

        }
        try {
            client.remove_trigger(curMultilogId, triggerName);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public RPCRecordBatchBuilder getBatchBuilder() {
        return new RPCRecordBatchBuilder();
    }

    public void write(ByteBuffer record) {
        if (curMultilogId == -1) {
            return;
        }
        if (record.array().length != curSchema.getRecordSize()) {
            return;
        }
        try {
            client.append(curMultilogId, record);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public ByteBuffer read(long offset) {
        if (curMultilogId == -1) {
            return null;
        }
        try {
            return client.read(curMultilogId, offset, 1);
        } catch (TException e) {
            e.printStackTrace();
        }
        return null;
    }

    public String getAggregate(String aggregateName, long beginMs, long endMs) {
        if (curMultilogId == -1) {

        }
        try {
            return client.query_aggregate(curMultilogId, aggregateName, beginMs, endMs);
        } catch (TException e) {
            e.printStackTrace();
        }
        return null;
    }

    public RecordStream execute_filter(String filterExpr) {
        if (curMultilogId == -1) {

        }
        try {
            rpc_iterator_handle handle = client.adhoc_filter(curMultilogId, filterExpr);
            return new RecordStream(curMultilogId, curSchema, client, handle);
        } catch (TException e) {
            e.printStackTrace();
        }
        return null;
    }

    public RecordStream queryFilter(String filterName, long beginMs, long endMs, String filterExpr) {
        if (curMultilogId == -1)  {

        }
        if (filterExpr.equals("")) {
            try {
                rpc_iterator_handle handle = client.predef_filter(curMultilogId, filterName, beginMs, endMs);
                return new RecordStream(curMultilogId, curSchema, client, handle);
            } catch (TException e) {
                e.printStackTrace();
            }
        } else {
            try {
                rpc_iterator_handle handle = client.combined_filter(curMultilogId, filterName, filterExpr, beginMs, endMs);
                return new RecordStream(curMultilogId, curSchema, client, handle);
            } catch (TException e) {
                e.printStackTrace();
            }
        }
        return null;
    }

    public AlertStream getAlerts(long beginMs, long endMs, String triggerName) {
        if (curMultilogId == -1) {

        }
        if (triggerName.equals("")) {
            try {
                rpc_iterator_handle handle = client.alerts_by_time(curMultilogId, beginMs, endMs);
                return new AlertStream(curMultilogId, curSchema, client, handle);
            } catch (TException e) {
                e.printStackTrace();
            }
        } else {
            try {
                rpc_iterator_handle handle = client.alerts_by_time(curMultilogId, beginMs, endMs);
                return new AlertStream(curMultilogId, curSchema, client, handle);
            } catch (TException e) {
                e.printStackTrace();
            }
        }
        return null;
    }


    public long numRecords() {
        if (curMultilogId == -1) {

        }
        try {
            return client.num_records(curMultilogId);
        } catch (TException e) {
            e.printStackTrace();
        }
        return -1;
    }
}

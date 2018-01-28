package confluo.rpc;

import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TTransport;
import org.apache.thrift.transport.TSocket;

import java.nio.ByteBuffer;
import java.util.List;

public class rpc_client {

    private TSocket socket;
    private TTransport transport;
    private TBinaryProtocol protocol;
    private rpc_service.Client client;
    private long cur_multilog_id;
    private rpc_atomic_multilog_info info;
    private schema cur_schema;

    public rpc_client(String host, int port) {
        connect(host, port);
        cur_multilog_id = -1;
    }

    public void close() {
        disconnect();
    }

    public void connect(String host, int port) {
        //socket = new TSocket(host, port);
        transport = new TSocket(host, port);//TFramedTransport(socket);
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

    public void create_atomic_multilog(String atomic_multilog_name, schema schema, rpc_storage_mode storage_mode) {
        this.cur_schema = schema;
        List<rpc_column> rpc_schema = rpc_type_conversions.convert_to_rpc_schema(schema);
        try {
            cur_multilog_id = client.create_atomic_multilog(atomic_multilog_name, rpc_schema, storage_mode);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void set_current_atomic_multilog(String atomic_multilog_name) {
        try {
            info = client.get_atomic_multilog_info(atomic_multilog_name);
        } catch (TException e) {
            e.printStackTrace();
        }
        cur_schema = rpc_type_conversions.convert_to_schema(info.get_schema());
        cur_multilog_id = info.get_id();
    }

    public void remove_atomic_multilog() {
        if (cur_multilog_id == -1) {

        }
        try {
            client.remove_atomic_multilog(cur_multilog_id);
            cur_multilog_id = -1;
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void add_index(String field_name, double bucket_size) {
        if (cur_multilog_id == -1) {

        }
        try {
            client.add_index(cur_multilog_id, field_name, bucket_size);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void remove_index(String field_name) {
        if (cur_multilog_id == -1) {

        }
        try {
            client.remove_index(cur_multilog_id, field_name);
        } catch (TException e) {
            e.printStackTrace();
        }
    }


    public void add_filter(String filter_name, String filter_expr) {
        if (cur_multilog_id == -1) {

        }
        try {
            client.add_filter(cur_multilog_id, filter_name, filter_expr);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void remove_filter(String filter_name) {
        if (cur_multilog_id == -1) {

        }
        try {
            client.remove_filter(cur_multilog_id, filter_name);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void add_aggregate(String aggregate_name, String filter_name, String aggregate_expr) {
        if (cur_multilog_id == -1) {

        }
        try {
            client.add_aggregate(cur_multilog_id, aggregate_name, filter_name, aggregate_expr);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void remove_aggregate(String aggregate_name) {
        if (cur_multilog_id == -1) {

        }
        try {
            client.remove_aggregate(cur_multilog_id, aggregate_name);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void install_trigger(String trigger_name, String trigger_expr) {
        if (cur_multilog_id == -1) {

        }
        try {
            client.add_trigger(cur_multilog_id, trigger_name, trigger_expr);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public void remove_trigger(String trigger_name) {
        if (cur_multilog_id == -1) {

        }
        try {
            client.remove_trigger(cur_multilog_id, trigger_name);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public rpc_record_batch_builder get_batch_builder() {
        return new rpc_record_batch_builder();
    }

    public void write(ByteBuffer record) {
        if (cur_multilog_id == -1) {

        }
        if (record.array().length != cur_schema.get_record_size()) {

        }
        try {
            client.append(cur_multilog_id, record);
        } catch (TException e) {
            e.printStackTrace();
        }
    }

    public ByteBuffer read(long offset) {
        if (cur_multilog_id == -1) {

        }
        try {
            return client.read(cur_multilog_id, offset, cur_schema.get_record_size());
        } catch (TException e) {
            e.printStackTrace();
        }
        return null;
    }

    public String get_aggregate(String aggregate_name, long begin_ms, long end_ms) {
        if (cur_multilog_id == -1) {

        }
        try {
            return client.query_aggregate(cur_multilog_id, aggregate_name, begin_ms, end_ms);
        } catch (TException e) {
            e.printStackTrace();
        }
        return null;
    }

    public record_stream execute_filter(String filter_expr) {
        if (cur_multilog_id == -1) {

        }
        try {
            rpc_iterator_handle handle = client.adhoc_filter(cur_multilog_id, filter_expr);
            return new record_stream(cur_multilog_id, cur_schema, client, handle);
        } catch (TException e) {
            e.printStackTrace();
        }
        return null;
    }

    public record_stream query_filter(String filter_name, long begin_ms, long end_ms, String filter_expr) {
        if (cur_multilog_id == -1)  {

        }
        if (filter_expr.equals("")) {
            try {
                rpc_iterator_handle handle = client.predef_filter(cur_multilog_id, filter_name, begin_ms, end_ms);
                return new record_stream(cur_multilog_id, cur_schema, client, handle);
            } catch (TException e) {
                e.printStackTrace();
            }
        } else {
            try {
                rpc_iterator_handle handle = client.combined_filter(cur_multilog_id, filter_name, filter_expr, begin_ms, end_ms);
                return new record_stream(cur_multilog_id, cur_schema, client, handle);
            } catch (TException e) {
                e.printStackTrace();
            }
        }
        return null;
    }

    public alert_stream get_alerts(long begin_ms, long end_ms, String trigger_name) {
        if (cur_multilog_id == -1) {

        }
        if (trigger_name.equals("")) {
            try {
                rpc_iterator_handle handle = client.alerts_by_time(cur_multilog_id, begin_ms, end_ms);
                return new alert_stream(cur_multilog_id, cur_schema, client, handle);
            } catch (TException e) {
                e.printStackTrace();
            }
        } else {
            try {
                rpc_iterator_handle handle = client.alerts_by_time(cur_multilog_id, begin_ms, end_ms);
                return new alert_stream(cur_multilog_id, cur_schema, client, handle);
            } catch (TException e) {
                e.printStackTrace();
            }
        }
        return null;
    }


    public long num_records() {
        if (cur_multilog_id == -1) {

        }
        try {
            return client.num_records(cur_multilog_id);
        } catch (TException e) {
            e.printStackTrace();
        }
        return -1;
    }
}

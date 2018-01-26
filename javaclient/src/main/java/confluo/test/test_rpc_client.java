package confluo.test;

import confluo.rpc.*;

import java.nio.ByteBuffer;
import java.util.List;

public class test_rpc_client {

    public void wait_till_server_ready() {
        boolean check = true;
        while (check) {
            rpc_client c = new rpc_client("127.0.0.1", 9090);
            check = false;
        }
    }

    public void start_server() {

    }

    public void stop_server() {

    }

    public void test_concurrent_connections() {

    }

    public void test_create_atomic_multilog() {
        start_server();
        rpc_client client = new rpc_client("127.0.0.1", 9090);
        schema_builder builder = new schema_builder();
        schema multilog_schema = new schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build());
        client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.IN_MEMORY);
        client.disconnect();
        stop_server();
    }

    public void test_read_write_in_memory() {
        start_server();
        rpc_client client = new rpc_client("127.0.0.1", 9090);
        schema_builder builder = new schema_builder();
        schema multilog_schema = new schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build());
        client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.IN_MEMORY);
        ByteBuffer data = ByteBuffer.allocate(Long.SIZE / Byte.SIZE + 8);
        data.putLong(System.nanoTime());
        data.putChar('a');
        data.putChar('b');
        data.putChar('c');
        data.putChar('d');
        data.putChar('e');
        data.putChar('f');
        data.putChar('g');
        data.putChar('h');

        client.write(data);
        ByteBuffer buf = client.read(0);
        for (int i = 0; i < 8; i++) {
            assert buf.get(i + 8) == (char) ('a' + i);
        }

        client.disconnect();
        stop_server();
    }

    public void test_read_write_durable_relaxed() {
        start_server();
        rpc_client client = new rpc_client("127.0.0.1", 9090);
        schema_builder builder = new schema_builder();
        schema multilog_schema = new schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build());
        client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.DURABLE_RELAXED);

        ByteBuffer data = ByteBuffer.allocate(Long.SIZE / Byte.SIZE + 8);
        data.putLong(System.nanoTime());
        data.putChar('a');
        data.putChar('b');
        data.putChar('c');
        data.putChar('d');
        data.putChar('e');
        data.putChar('f');
        data.putChar('g');
        data.putChar('h');

        client.write(data);
        ByteBuffer buf = client.read(0);
        for (int i = 0; i < 8; i++) {
            assert buf.get(i + 8) == (char) ('a' + i);
        }

        client.disconnect();
        stop_server();
    }

    public void test_read_write_durable() {
        start_server();
        rpc_client client = new rpc_client("127.0.0.1", 9090);
        schema_builder builder = new schema_builder();
        schema multilog_schema = new schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build());
        client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.DURABLE);

        ByteBuffer data = ByteBuffer.allocate(Long.SIZE / Byte.SIZE + 8);
        data.putLong(System.nanoTime());
        data.putChar('a');
        data.putChar('b');
        data.putChar('c');
        data.putChar('d');
        data.putChar('e');
        data.putChar('f');
        data.putChar('g');
        data.putChar('h');

        client.write(data);
        ByteBuffer buf = client.read(0);
        for (int i = 0; i < 8; i++) {
            assert buf.get(i + 8) == (char) ('a' + i);
        }

        client.disconnect();
        stop_server();
    }

    public void test_execute_filter() {
        start_server();
        rpc_client client = new rpc_client("127.0.0.1", 9090);
        schema multilog_schema = new schema(build_schema());
        client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.IN_MEMORY);

        client.add_index("a", 1);
        client.add_index("b", 2);
        client.add_index("c", 10);
        client.add_index("d", 2);
        client.add_index("e", 100);
        client.add_index("f", 0.1);
        client.add_index("g", 0.01);
        client.add_index("h", 1);
    }

    public List<column> build_schema() {
        schema_builder builder = new schema_builder();
        builder.add_column(data_types.BOOL_TYPE, "a");
        builder.add_column(data_types.CHAR_TYPE, "b");
        builder.add_column(data_types.SHORT_TYPE, "c");
        builder.add_column(data_types.INT_TYPE, "d");
        builder.add_column(data_types.LONG_TYPE, "e");
        builder.add_column(data_types.FLOAT_TYPE, "f");
        builder.add_column(data_types.DOUBLE_TYPE, "g");
        builder.add_column(data_types.STRING_TYPE(16), "h");
        return builder.build();
    }
}

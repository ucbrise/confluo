
import confluo.rpc.*;
import org.junit.Assert;
import org.junit.Test;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class TestRPCClient {

    private Process process;

    public void start_server() {
        String SERVER_EXECUTABLE = System.getenv("CONFLUO_SERVER_EXEC");
        if (SERVER_EXECUTABLE == null) {
            SERVER_EXECUTABLE = "confluod";
        }
        List<String> args = new ArrayList<>();
        args.add(SERVER_EXECUTABLE);
        args.add("--data-path");
        args.add("/tmp");
        ProcessBuilder pb = new ProcessBuilder(args);
        pb.redirectErrorStream(true);
        try {
            process = pb.start();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void stop_server() {
        process.destroy();
    }

    @Test
    public void test_concurrent_connections() {
        start_server();
        List<rpc_client> clients = new ArrayList<>();
        clients.add(new rpc_client("127.0.0.1", 9090));
        clients.add(new rpc_client("127.0.0.1", 9090));
        clients.add(new rpc_client("127.0.0.1", 9090));
        clients.add(new rpc_client("127.0.0.1", 9090));

        for (rpc_client c : clients) {
            c.disconnect();
        }

        stop_server();
    }

    @Test
    public void test_create_atomic_multilog() {
        start_server();
        rpc_client client = new rpc_client("127.0.0.1", 9090);
        schema_builder builder = new schema_builder();
        schema multilog_schema = new schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build());
        client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.IN_MEMORY);
        stop_server();
    }

    @Test
    public void test_read_write_in_memory() {
        start_server();
        rpc_client client = new rpc_client("127.0.0.1", 9090);
        schema_builder builder = new schema_builder();
        schema multilog_schema = new schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build());
        client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.IN_MEMORY);
        int size = Long.SIZE / Byte.SIZE + 8;
        ByteBuffer data = ByteBuffer.allocate(size);
        data.putLong(System.nanoTime());
        data.put("abcdefgh".getBytes());
        data.flip();

        client.write(data);
        ByteBuffer buf = client.read(0);
        for (int i = 0; i < 8; i++) {
            Assert.assertEquals((char) ('a' + i), buf.get(i + 8));
        }

        client.disconnect();
        stop_server();
    }

    @Test
    public void test_read_write_durable_relaxed() {
        start_server();
        rpc_client client = new rpc_client("127.0.0.1", 9090);
        schema_builder builder = new schema_builder();
        schema multilog_schema = new schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build());
        client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.DURABLE_RELAXED);

        ByteBuffer data = ByteBuffer.allocate(Long.SIZE / Byte.SIZE + 8);
        data.putLong(System.nanoTime());
        data.put("abcdefgh".getBytes());
        data.flip();

        client.write(data);
        ByteBuffer buf = client.read(0);
        for (int i = 0; i < 8; i++) {
            Assert.assertEquals((char) ('a' + i), buf.get(i + 8));
        }

        client.disconnect();
        stop_server();
    }

    @Test
    public void test_read_write_durable() {
        start_server();
        rpc_client client = new rpc_client("127.0.0.1", 9090);
        schema_builder builder = new schema_builder();
        schema multilog_schema = new schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build());
        client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.DURABLE);

        ByteBuffer data = ByteBuffer.allocate(Long.SIZE / Byte.SIZE + 8);
        data.putLong(System.nanoTime());
        data.put("abcdefgh".getBytes());
        data.flip();

        client.write(data);
        ByteBuffer buf = client.read(0);
        for (int i = 0; i < 8; i++) {
            Assert.assertEquals((char) ('a' + i), buf.get(i + 8));
        }

        client.disconnect();
        stop_server();
    }

    @Test
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

        client.write(pack_record(true, "0", (short) 0, 0, 0, 0.01f, 0.01, "abc"));
        client.write(pack_record(true, "1", (short) 10, 2, 1, 0.1f, 0.02, "defg"));
        client.write(pack_record(false, "2", (short) 20, 4, 10, 0.2f, 0.03, "hijkl"));
        client.write(pack_record(true, "3", (short) 30, 6, 100, 0.3f, 0.04, "mnopqr"));
        client.write(pack_record(false, "4", (short) 40, 8, 1000, 0.4f, 0.05, "stuvwx"));
        client.write(pack_record(true, "5", (short) 50, 10, 10000, 0.5f, 0.06, "yyy"));
        client.write(pack_record(false, "6", (short) 60, 12, 100000, 0.6f, 0.07, "zzz"));
        client.write(pack_record(true, "7", (short) 70, 14, 1000000, 0.7f, 0.08, "zzz"));

        int i = 0;
        for (record record : client.execute_filter("a == true")) {
            Assert.assertTrue(record.at(1).unpack() == 1);
            i += 1;
        }
        Assert.assertTrue(i == 4);

        i = 0;
        for (record record : client.execute_filter("b > 4")) {
            Assert.assertTrue(record.at(2).unpack() > 4);
            i += 1;
        }
        Assert.assertTrue(i == 3);

        i = 0;
        for (record record : client.execute_filter("c <= 30")) {
            Assert.assertTrue(record.at(3).unpack() <= 30);
            i += 1;
        }
        Assert.assertTrue(i == 4);

        i = 0;
        for (record record : client.execute_filter("d == 0")) {
            Assert.assertTrue(record.at(4).unpack() == 0);
            i += 1;
        }
        Assert.assertTrue(i == 1);

        i = 0;
        for (record record : client.execute_filter("e <= 100")) {
            Assert.assertTrue(record.at(5).unpack() <= 100);
            i += 1;
        }
        Assert.assertTrue(i == 4);

        i = 0;
        for (record record : client.execute_filter("f > 0.1")) {
            Assert.assertTrue(record.at(6).unpack() > 0.1);
            i += 1;
        }
        Assert.assertTrue(i == 6);

        i = 0;
        for (record record : client.execute_filter("g < 0.06")) {
            Assert.assertTrue(record.at(7).unpack() < 0.06);
            i += 1;
        }
        Assert.assertTrue(i == 5);

        i = 0;
        for (record record : client.execute_filter("h == zzz")) {
            //Assert.assertTrue(record.at(8).unpack());
            i += 1;
        }
        Assert.assertTrue(i == 2);

        i = 0;
        for (record record : client.execute_filter("a == true && b > 4")) {
            Assert.assertTrue(record.at(1).unpack() == 1);
            Assert.assertTrue(record.at(2).unpack() > 4);
            i += 1;
        }
        Assert.assertTrue(i == 2);

        i = 0;
        for (record record : client.execute_filter("a == true && (b > 4 || c <= 30)")) {
            Assert.assertTrue(record.at(1).unpack() == 1);
            Assert.assertTrue(record.at(2).unpack() > 4 || record.at(3).unpack() <= 30);
            i += 1;
        }
        Assert.assertTrue(i == 4);

        i = 0;
        for (record record : client.execute_filter("a == true && (b > 4 || f > 0.1)")) {
            Assert.assertTrue(record.at(1).unpack() == 1);
            Assert.assertTrue(record.at(2).unpack() > 4 || record.at(6).unpack() > 0.1);
            i += 1;
        }

        client.disconnect();
        stop_server();
    }

    @Test
    public void test_query_filter() {
        start_server();
        rpc_client client = new rpc_client("127.0.0.1", 9090);
        schema multilog_schema = new schema(build_schema());

        client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.IN_MEMORY);
        client.add_filter("filter1", "a == true");
        client.add_filter("filter2", "b > 4");
        client.add_filter("filter3", "c <= 30");
        client.add_filter("filter4", "d == 0");
        client.add_filter("filter5", "e <= 100");
        client.add_filter("filter6", "f > 0.1");
        client.add_filter("filter7", "g < 0.06");
        client.add_filter("filter8", "h == zzz");

        client.add_aggregate("agg1", "filter1", "SUM(d)");
        client.add_aggregate("agg2", "filter2", "SUM(d)");
        client.add_aggregate("agg3", "filter3", "SUM(d)");
        client.add_aggregate("agg4", "filter4", "SUM(d)");
        client.add_aggregate("agg5", "filter5", "SUM(d)");
        client.add_aggregate("agg6", "filter6", "SUM(d)");
        client.add_aggregate("agg7", "filter7", "SUM(d)");
        client.add_aggregate("agg8", "filter8", "SUM(d)");
        client.install_trigger("trigger1", "agg1 >= 10");
        client.install_trigger("trigger2", "agg2 >= 10");
        client.install_trigger("trigger3", "agg3 >= 10");
        client.install_trigger("trigger4", "agg4 >= 10");
        client.install_trigger("trigger5", "agg5 >= 10");
        client.install_trigger("trigger6", "agg6 >= 10");
        client.install_trigger("trigger7", "agg7 >= 10");
        client.install_trigger("trigger8", "agg8 >= 10");

        long now = System.nanoTime();
        long begin_ms = time_block(now);
        long end_ms = time_block(now);

        client.write(pack_record_time(now, false, '0', (short) 0, 0, 0, 0.0f, 0.01, "abc"));
        client.write(pack_record_time(now, true, '1', (short) 10, 2, 1, 0.1f, 0.02, "defg"));
        client.write(pack_record_time(now, false, '2', (short) 20, 4, 10, 0.2f, 0.03, "hijkl"));
        client.write(pack_record_time(now, true, '3', (short) 30, 6, 100, 0.3f, 0.04, "mnopqr"));
        client.write(pack_record_time(now, false, '4', (short) 40, 8, 1000, 0.4f, 0.05, "stuvwx"));
        client.write(pack_record_time(now, true, '5', (short) 50, 10, 10000, 0.5f, 0.06, "yyy"));
        client.write(pack_record_time(now, false, '6', (short) 60, 12, 100000, 0.6f, 0.07, "zzz"));
        client.write(pack_record_time(now, true, '7', (short) 70, 14, 1000000, 0.7f, 0.08, "zzz"));

        int i = 0;
        for (record record : client.query_filter("filter1", begin_ms, end_ms, "")) {
            Assert.assertEquals(1, record.at(1).get_data().get(0));
            i += 1;
        }
        Assert.assertEquals(4, i);

        i = 0;
        for (record record : client.query_filter("filter2", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(2).unpack() > 4);
            i += 1;
        }
        Assert.assertEquals(3, i);

        i = 0;
        for (record record : client.query_filter("filter3", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(3).unpack() <= 30);
            i += 1;
        }
        Assert.assertEquals(4, i);

        i = 0;
        for (record record : client.query_filter("filter4", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(4).unpack() == 0);
            i += 1;
        }
        Assert.assertEquals(1, i);

        i = 0;
        for (record record : client.query_filter("filter5", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(5).unpack() <= 100);
            i += 1;
        }
        Assert.assertEquals(4, i);

        i = 0;
        for (record record : client.query_filter("filter6", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(6).unpack() > 0.1);
            i += 1;
        }
        Assert.assertEquals(6, i);

        i = 0;
        for (record record : client.query_filter("filter7", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(7).unpack() < 0.06);
            i += 1;
        }
        Assert.assertEquals(5, i);

        stop_server();
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

    public ByteBuffer pack_record(boolean a, String b, short c, int d, long e, float f, double g, String h) {
        int size = 8 + 1 + 1 + 2 + 4 + 8 + 4 + 8 + 16;
        ByteBuffer rec = ByteBuffer.allocate(size);
        rec.putLong(System.nanoTime());
        rec.put((byte) (a ? 1 : 0));
        rec.put(b.getBytes());
        rec.putShort(c);
        rec.putInt(d);
        rec.putLong(e);
        rec.putFloat(f);
        rec.putDouble(g);
        rec.put(h.getBytes());
        rec.flip();
        return rec;
    }

    public ByteBuffer pack_record_time(long ts, boolean a, char b, short c, int d, long e, float f, double g, String h) {
        int size = 8 + 1 + 2 + 2 + 4 + 8 + 4 + 8 + 16;
        ByteBuffer rec = ByteBuffer.allocate(size);
        rec.putLong(ts);
        rec.put((byte) (a ? 1 : 0));
        rec.putChar(b);
        rec.putShort(c);
        rec.putInt(d);
        rec.putLong(e);
        rec.putFloat(f);
        rec.putDouble(g);
        rec.put(h.getBytes());
        rec.flip();
        return rec;
    }

    public long time_block(long ts) {
        return (long) (ts / 1e6);
    }

}

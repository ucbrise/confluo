
import confluo.rpc.*;
import org.junit.Assert;
import org.junit.Test;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class TestRPCClient {

    private Process process;

    public void startServer() {
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

    public void stopServer() {
        process.destroy();
    }

    @Test
    public void testConcurrentConnections() {
        startServer();
        List<RPCClient> clients = new ArrayList<>();
        clients.add(new RPCClient("127.0.0.1", 9090));
        clients.add(new RPCClient("127.0.0.1", 9090));
        clients.add(new RPCClient("127.0.0.1", 9090));
        clients.add(new RPCClient("127.0.0.1", 9090));

        for (RPCClient c : clients) {
            c.disconnect();
        }

        stopServer();
    }

    @Test
    public void testCreateAtomicMultilog() {
        startServer();
        RPCClient client = new RPCClient("127.0.0.1", 9090);
        SchemaBuilder builder = new SchemaBuilder();
        Schema multilogSchema = new Schema(builder.addColumn(DataTypes.STRING_TYPE(8), "msg").build());
        client.createAtomicMultilog("my_multilog", multilogSchema, StorageId.IN_MEMORY);
        stopServer();
    }

    @Test
    public void testReadWriteInMemory() {
        startServer();
        RPCClient client = new RPCClient("127.0.0.1", 9090);
        SchemaBuilder builder = new SchemaBuilder();
        Schema multilogSchema = new Schema(builder.addColumn(DataTypes.STRING_TYPE(8), "msg").build());
        client.createAtomicMultilog("my_multilog", multilogSchema, StorageId.IN_MEMORY);
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
        stopServer();
    }

    @Test
    public void testReadWriteDurableRelaxed() {
        startServer();
        RPCClient client = new RPCClient("127.0.0.1", 9090);
        SchemaBuilder builder = new SchemaBuilder();
        Schema multilogSchema = new Schema(builder.addColumn(DataTypes.STRING_TYPE(8), "msg").build());
        client.createAtomicMultilog("my_multilog", multilogSchema, StorageId.DURABLE_RELAXED);

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
        stopServer();
    }

    @Test
    public void testReadWriteDurable() {
        startServer();
        RPCClient client = new RPCClient("127.0.0.1", 9090);
        SchemaBuilder builder = new SchemaBuilder();
        Schema multilogSchema = new Schema(builder.addColumn(DataTypes.STRING_TYPE(8), "msg").build());
        client.createAtomicMultilog("my_multilog", multilogSchema, StorageId.DURABLE);

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
        stopServer();
    }

    @Test
    public void testExecuteFilter() {
        startServer();
        RPCClient client = new RPCClient("127.0.0.1", 9090);
        Schema multilogSchema = new Schema(buildSchema());
        client.createAtomicMultilog("my_multilog", multilogSchema, StorageId.IN_MEMORY);

        client.addIndex("a", 1);
        client.addIndex("b", 2);
        client.addIndex("c", 10);
        client.addIndex("d", 2);
        client.addIndex("e", 100);
        client.addIndex("f", 0.1);
        client.addIndex("g", 0.01);
        client.addIndex("h", 1);

        client.write(packRecord(true, "0", (short) 0, 0, 0, 0.01f, 0.01, "abc"));
        client.write(packRecord(true, "1", (short) 10, 2, 1, 0.1f, 0.02, "defg"));
        client.write(packRecord(false, "2", (short) 20, 4, 10, 0.2f, 0.03, "hijkl"));
        client.write(packRecord(true, "3", (short) 30, 6, 100, 0.3f, 0.04, "mnopqr"));
        client.write(packRecord(false, "4", (short) 40, 8, 1000, 0.4f, 0.05, "stuvwx"));
        client.write(packRecord(true, "5", (short) 50, 10, 10000, 0.5f, 0.06, "yyy"));
        client.write(packRecord(false, "6", (short) 60, 12, 100000, 0.6f, 0.07, "zzz"));
        client.write(packRecord(true, "7", (short) 70, 14, 1000000, 0.7f, 0.08, "zzz"));

        int i = 0;
        for (Record record : client.executeFilter("a == true")) {
            Assert.assertTrue(record.at(1).unpack() == 1);
            i += 1;
        }
        Assert.assertTrue(i == 4);

        i = 0;
        for (Record record : client.executeFilter("b > 4")) {
            Assert.assertTrue(record.at(2).unpack() > 4);
            i += 1;
        }
        Assert.assertTrue(i == 3);

        i = 0;
        for (Record record : client.executeFilter("c <= 30")) {
            Assert.assertTrue(record.at(3).unpack() <= 30);
            i += 1;
        }
        Assert.assertTrue(i == 4);

        i = 0;
        for (Record record : client.executeFilter("d == 0")) {
            Assert.assertTrue(record.at(4).unpack() == 0);
            i += 1;
        }
        Assert.assertTrue(i == 1);

        i = 0;
        for (Record record : client.executeFilter("e <= 100")) {
            Assert.assertTrue(record.at(5).unpack() <= 100);
            i += 1;
        }
        Assert.assertTrue(i == 4);

        i = 0;
        for (Record record : client.executeFilter("f > 0.1")) {
            Assert.assertTrue(record.at(6).unpack() > 0.1);
            i += 1;
        }
        Assert.assertTrue(i == 6);

        i = 0;
        for (Record record : client.executeFilter("g < 0.06")) {
            Assert.assertTrue(record.at(7).unpack() < 0.06);
            i += 1;
        }
        Assert.assertTrue(i == 5);

        i = 0;
        for (Record record : client.executeFilter("h == zzz")) {
            //Assert.assertTrue(Record.at(8).unpack());
            i += 1;
        }
        Assert.assertTrue(i == 2);

        i = 0;
        for (Record record : client.executeFilter("a == true && b > 4")) {
            Assert.assertTrue(record.at(1).unpack() == 1);
            Assert.assertTrue(record.at(2).unpack() > 4);
            i += 1;
        }
        Assert.assertTrue(i == 2);

        i = 0;
        for (Record record : client.executeFilter("a == true && (b > 4 || c <= 30)")) {
            Assert.assertTrue(record.at(1).unpack() == 1);
            Assert.assertTrue(record.at(2).unpack() > 4 || record.at(3).unpack() <= 30);
            i += 1;
        }
        Assert.assertTrue(i == 4);

        i = 0;
        for (Record record : client.executeFilter("a == true && (b > 4 || f > 0.1)")) {
            Assert.assertTrue(record.at(1).unpack() == 1);
            Assert.assertTrue(record.at(2).unpack() > 4 || record.at(6).unpack() > 0.1);
            i += 1;
        }

        client.disconnect();
        stopServer();
    }

    @Test
    public void testQueryFilter() {
        startServer();
        RPCClient client = new RPCClient("127.0.0.1", 9090);
        Schema multilogSchema = new Schema(buildSchema());

        client.createAtomicMultilog("my_multilog", multilogSchema, StorageId.IN_MEMORY);
        client.addFilter("filter1", "a == true");
        client.addFilter("filter2", "b > 4");
        client.addFilter("filter3", "c <= 30");
        client.addFilter("filter4", "d == 0");
        client.addFilter("filter5", "e <= 100");
        client.addFilter("filter6", "f > 0.1");
        client.addFilter("filter7", "g < 0.06");
        client.addFilter("filter8", "h == zzz");

        client.addAggregate("agg1", "filter1", "SUM(d)");
        client.addAggregate("agg2", "filter2", "SUM(d)");
        client.addAggregate("agg3", "filter3", "SUM(d)");
        client.addAggregate("agg4", "filter4", "SUM(d)");
        client.addAggregate("agg5", "filter5", "SUM(d)");
        client.addAggregate("agg6", "filter6", "SUM(d)");
        client.addAggregate("agg7", "filter7", "SUM(d)");
        client.addAggregate("agg8", "filter8", "SUM(d)");
        client.installTrigger("trigger1", "agg1 >= 10");
        client.installTrigger("trigger2", "agg2 >= 10");
        client.installTrigger("trigger3", "agg3 >= 10");
        client.installTrigger("trigger4", "agg4 >= 10");
        client.installTrigger("trigger5", "agg5 >= 10");
        client.installTrigger("trigger6", "agg6 >= 10");
        client.installTrigger("trigger7", "agg7 >= 10");
        client.installTrigger("trigger8", "agg8 >= 10");

        long now = System.nanoTime();
        long begin_ms = timeBlock(now);
        long end_ms = timeBlock(now);

        client.write(packRecordTime(now, false, '0', (short) 0, 0, 0, 0.0f, 0.01, "abc"));
        client.write(packRecordTime(now, true, '1', (short) 10, 2, 1, 0.1f, 0.02, "defg"));
        client.write(packRecordTime(now, false, '2', (short) 20, 4, 10, 0.2f, 0.03, "hijkl"));
        client.write(packRecordTime(now, true, '3', (short) 30, 6, 100, 0.3f, 0.04, "mnopqr"));
        client.write(packRecordTime(now, false, '4', (short) 40, 8, 1000, 0.4f, 0.05, "stuvwx"));
        client.write(packRecordTime(now, true, '5', (short) 50, 10, 10000, 0.5f, 0.06, "yyy"));
        client.write(packRecordTime(now, false, '6', (short) 60, 12, 100000, 0.6f, 0.07, "zzz"));
        client.write(packRecordTime(now, true, '7', (short) 70, 14, 1000000, 0.7f, 0.08, "zzz"));

        int i = 0;
        for (Record record : client.queryFilter("filter1", begin_ms, end_ms, "")) {
            Assert.assertEquals(1, record.at(1).getData().get(0));
            i += 1;
        }
        Assert.assertEquals(4, i);

        i = 0;
        for (Record record : client.queryFilter("filter2", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(2).unpack() > 4);
            i += 1;
        }
        Assert.assertEquals(3, i);

        i = 0;
        for (Record record : client.queryFilter("filter3", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(3).unpack() <= 30);
            i += 1;
        }
        Assert.assertEquals(4, i);

        i = 0;
        for (Record record : client.queryFilter("filter4", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(4).unpack() == 0);
            i += 1;
        }
        Assert.assertEquals(1, i);

        i = 0;
        for (Record record : client.queryFilter("filter5", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(5).unpack() <= 100);
            i += 1;
        }
        Assert.assertEquals(4, i);

        i = 0;
        for (Record record : client.queryFilter("filter6", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(6).unpack() > 0.1);
            i += 1;
        }
        Assert.assertEquals(6, i);

        i = 0;
        for (Record record : client.queryFilter("filter7", begin_ms, end_ms, "")) {
            Assert.assertTrue(record.at(7).unpack() < 0.06);
            i += 1;
        }
        Assert.assertEquals(5, i);

        stopServer();
    }

    public List<Column> buildSchema() {
        SchemaBuilder builder = new SchemaBuilder();
        builder.addColumn(DataTypes.BOOL_TYPE, "a");
        builder.addColumn(DataTypes.CHAR_TYPE, "b");
        builder.addColumn(DataTypes.SHORT_TYPE, "c");
        builder.addColumn(DataTypes.INT_TYPE, "d");
        builder.addColumn(DataTypes.LONG_TYPE, "e");
        builder.addColumn(DataTypes.FLOAT_TYPE, "f");
        builder.addColumn(DataTypes.DOUBLE_TYPE, "g");
        builder.addColumn(DataTypes.STRING_TYPE(16), "h");
        return builder.build();
    }

    public ByteBuffer packRecord(boolean a, String b, short c, int d, long e, float f, double g, String h) {
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

    public ByteBuffer packRecordTime(long ts, boolean a, char b, short c, int d, long e, float f, double g, String h) {
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

    public long timeBlock(long ts) {
        return (long) (ts / 1e6);
    }

}

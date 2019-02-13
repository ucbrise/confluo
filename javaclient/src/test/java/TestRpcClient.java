import confluo.rpc.*;
import org.apache.thrift.TException;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import static org.junit.Assert.assertEquals;

public class TestRpcClient {

  private Process process;
  private final String MULTILOG_NAME = "my_multilog";
  private final String HOST = "127.0.0.1";
  private final int PORT = 9090;

  private void startServer() throws IOException, InterruptedException {
    String SERVER_EXECUTABLE = System.getenv("CONFLUO_SERVER_EXEC");
    if (SERVER_EXECUTABLE == null) {
      SERVER_EXECUTABLE = "confluod";
    }
    ProcessBuilder pb = new ProcessBuilder(SERVER_EXECUTABLE, "--data-path", "/tmp/javaclient");
    File log = new File("/tmp/java_test.txt");
    pb.redirectErrorStream(true);
    pb.redirectOutput(ProcessBuilder.Redirect.appendTo(log));
    process = pb.start();
    waitTillServerReady();
  }

  @Before
  public void setUp() throws IOException, InterruptedException {
    startServer();
  }

  @After
  public void tearDown() throws InterruptedException {
    stopServer();
  }

  private void waitTillServerReady() throws InterruptedException {
    boolean check = true;
    int RETRY_SLEEP_DURATION_MS = 100;
    while (check) {
      try {
        RpcClient c = new RpcClient(HOST, PORT);
        c.close();
        check = false;
      } catch (TException e) {
        Thread.sleep(RETRY_SLEEP_DURATION_MS);
      }
    }
  }

  private void stopServer() throws InterruptedException {
    process.destroy();
    process.waitFor();
  }

  @Test
  public void testConcurrentConnections() throws TException {
    List<RpcClient> clients = new ArrayList<>();
    clients.add(new RpcClient(HOST, PORT));
    clients.add(new RpcClient(HOST, PORT));
    clients.add(new RpcClient(HOST, PORT));
    clients.add(new RpcClient(HOST, PORT));

    for (RpcClient c : clients) {
      c.disconnect();
    }
  }

  @Test
  public void testCreateAtomicMultilog() throws TException {
    RpcClient client = new RpcClient(HOST, PORT);
    client.createAtomicMultilog(MULTILOG_NAME, "{ msg: STRING(8) }", StorageMode.IN_MEMORY);
    client.disconnect();
  }

  private void readWrite(rpc_storage_mode mode) throws TException {
    RpcClient client = new RpcClient(HOST, PORT);
    client.createAtomicMultilog(MULTILOG_NAME, "{ msg: STRING(8) }", mode);

    client.append("abcdefgh");
    Record record = client.read(0);
    assertEquals("abcdefgh", record.get(1).asString());

    client.disconnect();
  }

  private void batchWriteRead(rpc_storage_mode mode) throws TException {
    RpcClient client = new RpcClient(HOST, PORT);
    client.createAtomicMultilog(MULTILOG_NAME, "{ msg: STRING(8) }", mode);
    RecordBatchBuilder batchBuilder = client.getBatchBuilder();
    // batchBuilder.addRecord();
    try {
      batchBuilder.addRecord(client.getSchema().pack(false, "abcdef01"));
      batchBuilder.addRecord(client.getSchema().pack(false, "abcdef02"));
      batchBuilder.addRecord(client.getSchema().pack(false, "abcdef03"));
      client.appendBatch(batchBuilder.getBatch());
      Record record = client.read(0);
      assertEquals("abcdef01", record.get(1).asString());
      record = client.read(client.getSchema().getRecordSize());
      assertEquals("abcdef02", record.get(1).asString());
      record = client.read(2 * client.getSchema().getRecordSize());
      assertEquals("abcdef03", record.get(1).asString());
    } catch (IOException e) {
      throw new TException(e);
    }
    client.disconnect();
  }

  @Test
  public void testBatchWriteReadInMemory() throws TException {
    batchWriteRead(StorageMode.IN_MEMORY);
  }

  @Test
  public void testReadWriteInMemory() throws TException {
    readWrite(StorageMode.IN_MEMORY);
  }

  @Test
  public void testReadWriteDurableRelaxed() throws TException {
    readWrite(StorageMode.DURABLE_RELAXED);
  }

  @Test
  public void testReadWriteDurable() throws TException {
    readWrite(StorageMode.DURABLE);
  }

  @Test
  public void testExecuteFilter() throws TException {
    RpcClient client = new RpcClient(HOST, PORT);
    String multilogSchema = "{\n" +
        "  \"a\": \"BOOL\",\n" +
        "  \"b\": \"CHAR\",\n" +
        "  \"c\": \"SHORT\",\n" +
        "  \"d\": \"INT\",\n" +
        "  \"e\": \"LONG\",\n" +
        "  \"f\": \"FLOAT\",\n" +
        "  \"g\": \"DOUBLE\",\n" +
        "  \"h\": \"STRING(16)\"\n" +
        "}";
    client.createAtomicMultilog(MULTILOG_NAME, multilogSchema, StorageMode.IN_MEMORY);

    client.addIndex("a", 1);
    client.addIndex("b", 1);
    client.addIndex("c", 10);
    client.addIndex("d", 2);
    client.addIndex("e", 100);
    client.addIndex("f", 0.1);
    client.addIndex("g", 0.01);
    client.addIndex("h", 1);

    client.append("false", "0", "0", "0", "0", "0.0f", "0.01", "abc");
    client.append("true", "1", "10", "2", "1", "0.1f", "0.02", "defg");
    client.append("false", "2", "20", "4", "10", "0.2f", "0.03", "hijkl");
    client.append("true", "3", "30", "6", "100", "0.3f", "0.04", "mnopqr");
    client.append("false", "4", "40", "8", "1000", "0.4f", "0.05", "stuvwx");
    client.append("true", "5", "50", "10", "10000", "0.5f", "0.06", "yyy");
    client.append("false", "6", "60", "12", "100000", "0.6f", "0.07", "zzz");
    client.append("true", "7", "70", "14", "1000000", "0.7f", "0.08", "zzz");

    int i = 0;
    Iterator<Record> it;
    it = client.executeFilter("a == true");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(1).asBoolean());
      i += 1;
    }
    assertEquals(4, i);

    i = 0;
    it = client.executeFilter("b > 4");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(2).asChar() > '4');
      i += 1;
    }
    assertEquals(3, i);

    i = 0;
    it = client.executeFilter("c <= 30");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(3).asShort() <= 30);
      i += 1;
    }
    assertEquals(4, i);

    i = 0;
    it = client.executeFilter("d == 0");
    while (it.hasNext()) {
      Record record = it.next();
      assertEquals(0, record.get(4).asInt());
      i += 1;
    }
    assertEquals(1, i);

    i = 0;
    it = client.executeFilter("e <= 100");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(5).asLong() <= 100);
      i += 1;
    }
    assertEquals(4, i);

    i = 0;
    it = client.executeFilter("f > 0.1");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(6).asFloat() > 0.1);
      i += 1;
    }
    assertEquals(6, i);

    i = 0;
    it = client.executeFilter("g < 0.06");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(7).asDouble() < 0.06);
      i += 1;
    }
    assertEquals(5, i);

    i = 0;
    it = client.executeFilter("h == zzz");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertEquals("zzz", record.get(8).asString());
      i += 1;
    }
    assertEquals(2, i);

    i = 0;
    it = client.executeFilter("a == true && b > 4");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(1).asBoolean());
      Assert.assertTrue(record.get(2).asChar() > '4');
      i += 1;
    }
    assertEquals(2, i);

    i = 0;
    it = client.executeFilter("a == true && (b > 4 || c <= 30)");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(1).asBoolean());
      Assert.assertTrue(record.get(2).asChar() > '4' || record.get(3).asShort() <= 30);
      i += 1;
    }
    assertEquals(4, i);

    i = 0;
    it = client.executeFilter("a == true && (b > 4 || f > 0.1)");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(1).asBoolean());
      Assert.assertTrue(record.get(2).asChar() > '4' || record.get(6).asFloat() > 0.1);
      i += 1;
    }
    assertEquals(3, i);

    client.disconnect();
  }

  @Test
  public void testQueryFilter() throws TException {
    RpcClient client = new RpcClient(HOST, PORT);
    String multilogSchema = "{\n" +
        "  \"a\": \"BOOL\",\n" +
        "  \"b\": \"CHAR\",\n" +
        "  \"c\": \"SHORT\",\n" +
        "  \"d\": \"INT\",\n" +
        "  \"e\": \"LONG\",\n" +
        "  \"f\": \"FLOAT\",\n" +
        "  \"g\": \"DOUBLE\",\n" +
        "  \"h\": \"STRING(16)\"\n" +
        "}";
    client.createAtomicMultilog(MULTILOG_NAME, multilogSchema, StorageMode.IN_MEMORY);
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
    long beginMs = timeBlock(now);
    long endMs = timeBlock(now);
    String nowStr = String.valueOf(now);

    client.append(nowStr, "false", "0", "0", "0", "0", "0.0f", "0.01", "abc");
    client.append(nowStr, "true", "1", "10", "2", "1", "0.1f", "0.02", "defg");
    client.append(nowStr, "false", "2", "20", "4", "10", "0.2f", "0.03", "hijkl");
    client.append(nowStr, "true", "3", "30", "6", "100", "0.3f", "0.04", "mnopqr");
    client.append(nowStr, "false", "4", "40", "8", "1000", "0.4f", "0.05", "stuvwx");
    client.append(nowStr, "true", "5", "50", "10", "10000", "0.5f", "0.06", "yyy");
    client.append(nowStr, "false", "6", "60", "12", "100000", "0.6f", "0.07", "zzz");
    client.append(nowStr, "true", "7", "70", "14", "1000000", "0.7f", "0.08", "zzz");

    int i = 0;
    Iterator<Record> it;
    it = client.queryFilter("filter1", beginMs, endMs, "");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(1).asBoolean());
      i += 1;
    }
    assertEquals(4, i);

    i = 0;
    it = client.queryFilter("filter2", beginMs, endMs, "");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(2).asChar() > '4');
      i += 1;
    }
    assertEquals(3, i);

    i = 0;
    it = client.queryFilter("filter3", beginMs, endMs, "");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(3).asShort() <= 30);
      i += 1;
    }
    assertEquals(4, i);

    i = 0;
    it = client.queryFilter("filter4", beginMs, endMs, "");
    while (it.hasNext()) {
      Record record = it.next();
      assertEquals(0, record.get(4).asInt());
      i += 1;
    }
    assertEquals(1, i);

    i = 0;
    it = client.queryFilter("filter5", beginMs, endMs, "");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(5).asLong() <= 100);
      i += 1;
    }
    assertEquals(4, i);

    i = 0;
    it = client.queryFilter("filter6", beginMs, endMs, "");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(6).asFloat() > 0.1);
      i += 1;
    }
    assertEquals(6, i);

    i = 0;
    it = client.queryFilter("filter7", beginMs, endMs, "");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(7).asDouble() < 0.06);
      i += 1;
    }
    assertEquals(5, i);

    i = 0;
    it = client.queryFilter("filter8", beginMs, endMs, "");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertEquals("zzz", record.get(8).asString());
      i += 1;
    }
    assertEquals(2, i);

    i = 0;
    it = client.queryFilter("filter1", beginMs, endMs, "b > 4 || c <= 30");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(1).asBoolean());
      Assert.assertTrue(record.get(2).asChar() > '4' || record.get(3).asShort() <= 30);
      i += 1;
    }
    assertEquals(4, i);

    i = 0;
    it = client.queryFilter("filter1", beginMs, endMs, "b > 4 || f > 0.1");
    while (it.hasNext()) {
      Record record = it.next();
      Assert.assertTrue(record.get(1).asBoolean());
      Assert.assertTrue(record.get(2).asChar() > '4' || record.get(6).asFloat() > 0.1);
      i += 1;
    }
    assertEquals(3, i);

    String val1 = client.getAggregate("agg1", beginMs, endMs);
    assertEquals("double(32.000000)", val1);

    String val2 = client.getAggregate("agg2", beginMs, endMs);
    assertEquals("double(36.000000)", val2);

    String val3 = client.getAggregate("agg3", beginMs, endMs);
    assertEquals("double(12.000000)", val3);

    String val4 = client.getAggregate("agg4", beginMs, endMs);
    assertEquals("double(0.000000)", val4);

    String val5 = client.getAggregate("agg5", beginMs, endMs);
    assertEquals("double(12.000000)", val5);

    String val6 = client.getAggregate("agg6", beginMs, endMs);
    assertEquals("double(54.000000)", val6);

    String val7 = client.getAggregate("agg7", beginMs, endMs);
    assertEquals("double(20.000000)", val7);

    String val8 = client.getAggregate("agg8", beginMs, endMs);
    assertEquals("double(26.000000)", val8);

    client.disconnect();
  }

  private long timeBlock(long ts) {
    return (long) (ts / 1e6);
  }

}

package confluo.test;

import confluo.rpc.*;
import org.junit.Assert;
import org.junit.Test;

import java.io.*;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Semaphore;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.concurrent.Semaphore;

public class test_rpc_client {

    private Process process;

        public void wait_till_server_ready() {
            boolean check = true;
            while (check) {
                rpc_client c = new rpc_client("127.0.0.1", 9090);
                check = false;
            }
        }


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
                //InputStream is = process.getInputStream();
                //InputStreamReader isr = new InputStreamReader(is);
                //BufferedReader br = new BufferedReader(isr);
                //String line;
                /*while ((line = br.readLine()) != null) {
                    System.out.println(line);
                }*/
                //wait_till_server_ready();
                //System.out.println("Program terminated");
            //stop_server();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        public void stop_server() {
            process.destroy();
        }

        public void test_concurrent_connections() {

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
            ByteBuffer data = ByteBuffer.allocate(Long.SIZE / Byte.SIZE + 8 * (Character.SIZE / Byte.SIZE));
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
                Assert.assertEquals((char) ('a' + i), buf.get(i + 8));
            }

            client.disconnect();
            stop_server();
            process.destroyForcibly();
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

            client.write(pack_record(false, '0', (short) 0, 0, 0, 0.01f, 0.01, "abc"));
            client.write(pack_record(true, '1', (short) 10, 2, 1, 0.1f, 0.02, "defg"));
            client.write(pack_record(false, '2', (short) 20, 4, 10, 0.2f, 0.03, "hijkl"));
            client.write(pack_record(true, '3', (short) 30, 6, 100, 0.3f, 0.04, "mnopqr"));
            client.write(pack_record(false, '4', (short) 40, 8, 1000, 0.4f, 0.05, "stuvwx"));
            client.write(pack_record(true, '5', (short) 50, 10, 10000, 0.5f, 0.06, "yyy"));
            client.write(pack_record(false, '6', (short) 60, 12, 100000, 0.6f, 0.07, "zzz"));
            client.write(pack_record(true, '7', (short) 70, 14, 1000000, 0.7f, 0.08, "zzz"));

            int i = 0;
        /*for (record record : client.execute_filter("a == true")) {
            assert record.at(1).equals(true);
            i += 1;
        }

        i = 0;
        for (record record : client.execute_filter("b > 4")) {

            i += 1;
        }

        i = 0;
        for (record record : client.execute_filter("c <= 30")) {

            i += 1;
        }

        i = 0;
        for (record record : client.execute_filter("d == 0")) {

            i += 1;
        }

        i = 0;
        for (record record : client.execute_filter("e <= 100")) {

            i += 1;
        }

        i = 0;
        for (record record : client.execute_filter("f > 0.1")) {

            i += 1;
        }

        i = 0;
        for (record record : client.execute_filter("g < 0.06")) {

            i += 1;
        }

        i = 0;
        for (record record : client.execute_filter("h == zzz")) {

            i += 1;
        }

        i = 0;
        for (record record : client.execute_filter("a == true && b > 4")) {

            i += 1;
        }

        i = 0;
        for (record record : client.execute_filter("a == true && (b > 4 || c <= 30)")) {

            i += 1;
        }

        i = 0;
        for (record record : client.execute_filter("a == true && (b > 4 || f > 0.1)")) {

            i += 1;
        }*/

            client.disconnect();
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

        public ByteBuffer pack_record(boolean a, char b, short c, int d, long e, float f, double g, String h) {
            int size = 8 + 1 + 2 + 2 + 4 + 8 + 4 + 8 + 16;
            ByteBuffer rec = ByteBuffer.allocate(size);
            rec.putLong(System.nanoTime());
            rec.put((byte) (a ? 1 : 0));
            rec.putChar(b);
            rec.putShort(c);
            rec.putInt(d);
            rec.putLong(e);
            rec.putFloat(f);
            rec.putDouble(g);

            for (int i = 0; i < h.length(); i++) {
                rec.putChar(h.charAt(i));
            }

            return rec;
        }

}

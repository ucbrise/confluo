import os
import subprocess
import time
import unittest
from confluo.rpc.client import RpcClient
from confluo.rpc.storage import StorageMode
from thrift.transport import TTransport


class TestRpcClient(unittest.TestCase):
    SERVER_EXECUTABLE = os.getenv('CONFLUO_SERVER_EXEC', 'confluod')

    @staticmethod
    def wait_till_server_ready():
        check = True
        while check:
            try:
                c = RpcClient("127.0.0.1", 9090)
                check = False
            except TTransport.TTransportException as e:
                time.sleep(0.1)

    def start_server(self):
        self.server = subprocess.Popen([self.SERVER_EXECUTABLE, '--data-path', '/tmp'])
        self.wait_till_server_ready()

    def stop_server(self):
        pid = self.server.pid
        self.server.kill()
        self.server.wait()

    def test_concurrent_connections(self):
        self.start_server()
        clients = [RpcClient("127.0.0.1", 9090) for x in xrange(4)]

        try:
            for c in clients:
                c.disconnect()
        except:
            self.stop_server()
            raise

        self.stop_server()

    def test_create_atomic_multilog(self):
        self.start_server()
        client = RpcClient("127.0.0.1", 9090)

        try:
            client.create_atomic_multilog("my_multilog", '{ msg: STRING(8) }', StorageMode.IN_MEMORY)
        except:
            self.stop_server()
            raise

        client.disconnect()
        self.stop_server()

    def read_write(self, mode):
        self.start_server()
        client = RpcClient("127.0.0.1", 9090)

        try:
            client.create_atomic_multilog("my_multilog", '{ msg: STRING(8) }', mode)
            client.append(["abcdefgh"])
            rec = client.read(0)
            print rec
            self.assertTrue(rec[1] == "abcdefgh")
        except:
            self.stop_server()
            raise

        client.disconnect()
        self.stop_server()

    def test_read_write(self):
        self.read_write(StorageMode.IN_MEMORY)
        self.read_write(StorageMode.DURABLE_RELAXED)
        self.read_write(StorageMode.DURABLE)

    def test_execute_filter(self):

        self.start_server()
        client = RpcClient("127.0.0.1", 9090)

        try:
            multilog_schema = """{
                a: BOOL,
                b: CHAR,
                c: SHORT,
                d: INT,
                e: LONG,
                f: FLOAT,
                g: DOUBLE,
                h: STRING(16)
            }"""
            client.create_atomic_multilog("my_multilog", multilog_schema, StorageMode.IN_MEMORY)

            client.add_index("a", 1)
            client.add_index("b", 1)
            client.add_index("c", 10)
            client.add_index("d", 2)
            client.add_index("e", 100)
            client.add_index("f", 0.1)
            client.add_index("g", 0.01)
            client.add_index("h", 1)

            client.append([False, "0", 0, 0, 0, 0.0, 0.01, "abc"])
            client.append([True, "1", 10, 2, 1, 0.1, 0.02, "defg"])
            client.append([False, "2", 20, 4, 10, 0.2, 0.03, "hijkl"])
            client.append([True, "3", 30, 6, 100, 0.3, 0.04, "mnopqr"])
            client.append([False, "4", 40, 8, 1000, 0.4, 0.05, "stuvwx"])
            client.append([True, "5", 50, 10, 10000, 0.5, 0.06, "yyy"])
            client.append([False, "6", 60, 12, 100000, 0.6, 0.07, "zzz"])
            client.append([True, "7", 70, 14, 1000000, 0.7, 0.08, "zzz"])

            i = 0
            for record in client.execute_filter("a == true"):
                self.assertTrue(record[1])
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.execute_filter("b > 4"):
                self.assertTrue(record[2] > 4)
                i += 1
            self.assertTrue(i == 3)

            i = 0
            for record in client.execute_filter("c <= 30"):
                self.assertTrue(record[3] <= 30)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.execute_filter("d == 0"):
                self.assertTrue(record[4] == 0)
                i += 1
            self.assertTrue(i == 1)

            i = 0
            for record in client.execute_filter("e <= 100"):
                self.assertTrue(record[5] <= 100)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.execute_filter("f > 0.1"):
                self.assertTrue(record[6] > 0.1)
                i += 1
            self.assertTrue(i == 6)

            i = 0
            for record in client.execute_filter("g < 0.06"):
                self.assertTrue(record[7] < 0.06)
                i += 1
            self.assertTrue(i == 5)

            i = 0
            for record in client.execute_filter("h == zzz"):
                self.assertTrue(record[8][:3] == "zzz")
                i += 1
            self.assertTrue(i == 2)

            i = 0
            for record in client.execute_filter("a == true && b > 4"):
                self.assertTrue(record[1])
                self.assertTrue(record[2] > 4)
                i += 1
            self.assertTrue(i == 2)

            i = 0
            for record in client.execute_filter("a == true && (b > 4 || c <= 30)"):
                self.assertTrue(record[1])
                self.assertTrue(record[2] > 4 or record[3] <= 30)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.execute_filter("a == true && (b > 4 || f > 0.1)"):
                self.assertTrue(record[1])
                self.assertTrue(record[2] > 4 or record[6] > 0.1)
                i += 1
            self.assertTrue(i == 3)
        except:
            self.stop_server()
            raise

        client.disconnect()
        self.stop_server()

    def test_query_filter(self):

        self.start_server()
        client = RpcClient("127.0.0.1", 9090)

        try:
            multilog_schema = """{
                a: BOOL,
                b: CHAR,
                c: SHORT,
                d: INT,
                e: LONG,
                f: FLOAT,
                g: DOUBLE,
                h: STRING(16)
            }"""
            client.create_atomic_multilog("my_multilog", multilog_schema, StorageMode.IN_MEMORY)

            client.add_filter("filter1", "a == true")
            client.add_filter("filter2", "b > 4")
            client.add_filter("filter3", "c <= 30")
            client.add_filter("filter4", "d == 0")
            client.add_filter("filter5", "e <= 100")
            client.add_filter("filter6", "f > 0.1")
            client.add_filter("filter7", "g < 0.06")
            client.add_filter("filter8", "h == zzz")
            client.add_aggregate("agg1", "filter1", "SUM(d)")
            client.add_aggregate("agg2", "filter2", "SUM(d)")
            client.add_aggregate("agg3", "filter3", "SUM(d)")
            client.add_aggregate("agg4", "filter4", "SUM(d)")
            client.add_aggregate("agg5", "filter5", "SUM(d)")
            client.add_aggregate("agg6", "filter6", "SUM(d)")
            client.add_aggregate("agg7", "filter7", "SUM(d)")
            client.add_aggregate("agg8", "filter8", "SUM(d)")
            client.install_trigger("trigger1", "agg1 >= 10")
            client.install_trigger("trigger2", "agg2 >= 10")
            client.install_trigger("trigger3", "agg3 >= 10")
            client.install_trigger("trigger4", "agg4 >= 10")
            client.install_trigger("trigger5", "agg5 >= 10")
            client.install_trigger("trigger6", "agg6 >= 10")
            client.install_trigger("trigger7", "agg7 >= 10")
            client.install_trigger("trigger8", "agg8 >= 10")

            now = self.now_ns()
            beg_ms = self.time_block(now)
            end_ms = self.time_block(now)
            client.append([now, False, "0", 0, 0, 0, 0.0, 0.01, "abc"])
            client.append([now, True, "1", 10, 2, 1, 0.1, 0.02, "defg"])
            client.append([now, False, "2", 20, 4, 10, 0.2, 0.03, "hijkl"])
            client.append([now, True, "3", 30, 6, 100, 0.3, 0.04, "mnopqr"])
            client.append([now, False, "4", 40, 8, 1000, 0.4, 0.05, "stuvwx"])
            client.append([now, True, "5", 50, 10, 10000, 0.5, 0.06, "yyy"])
            client.append([now, False, "6", 60, 12, 100000, 0.6, 0.07, "zzz"])
            client.append([now, True, "7", 70, 14, 1000000, 0.7, 0.08, "zzz"])

            i = 0
            for record in client.query_filter("filter1", beg_ms, end_ms):
                self.assertTrue(record[1])
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.query_filter("filter2", beg_ms, end_ms):
                self.assertTrue(record[2] > 4)
                i += 1
            self.assertTrue(i == 3)

            i = 0
            for record in client.query_filter("filter3", beg_ms, end_ms):
                self.assertTrue(record[3] <= 30)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.query_filter("filter4", beg_ms, end_ms):
                self.assertTrue(record[4] == 0)
                i += 1
            self.assertTrue(i == 1)

            i = 0
            for record in client.query_filter("filter5", beg_ms, end_ms):
                self.assertTrue(record[5] <= 100)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.query_filter("filter6", beg_ms, end_ms):
                self.assertTrue(record[6] > 0.1)
                i += 1
            self.assertTrue(i == 6)

            i = 0
            for record in client.query_filter("filter7", beg_ms, end_ms):
                self.assertTrue(record[7] < 0.06)
                i += 1
            self.assertTrue(i == 5)

            i = 0
            for record in client.query_filter("filter8", beg_ms, end_ms):
                self.assertTrue(record[8][:3] == "zzz")
                i += 1
            self.assertTrue(i == 2)

            i = 0
            for record in client.query_filter("filter1", beg_ms, end_ms, "b > 4 || c <= 30"):
                self.assertTrue(record[1])
                self.assertTrue(record[2] > 4 or record[3] <= 30)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.query_filter("filter1", beg_ms, end_ms, "b > 4 || f > 0.1"):
                self.assertTrue(record[1])
                self.assertTrue(record[2] > 4 or record[6] > 0.1)
                i += 1
            self.assertTrue(i == 3)

            val1 = client.get_aggregate("agg1", beg_ms, end_ms)
            self.assertTrue("double(32.000000)" == val1)

            val2 = client.get_aggregate("agg2", beg_ms, end_ms)
            self.assertTrue("double(36.000000)" == val2)

            val3 = client.get_aggregate("agg3", beg_ms, end_ms)
            self.assertTrue("double(12.000000)" == val3)

            val4 = client.get_aggregate("agg4", beg_ms, end_ms)
            self.assertTrue("double(0.000000)" == val4)

            val5 = client.get_aggregate("agg5", beg_ms, end_ms)
            self.assertTrue("double(12.000000)" == val5)

            val6 = client.get_aggregate("agg6", beg_ms, end_ms)
            self.assertTrue("double(54.000000)" == val6)

            val7 = client.get_aggregate("agg7", beg_ms, end_ms)
            self.assertTrue("double(20.000000)" == val7)

            val8 = client.get_aggregate("agg8", beg_ms, end_ms)
            self.assertTrue("double(26.000000)" == val8)

        except:
            self.stop_server()
            raise

        client.disconnect()
        self.stop_server()

    @staticmethod
    def now_ns():
        return int(time.time() * 10 ** 6)

    @staticmethod
    def time_block(ts):
        return ts / 10 ** 6

import subprocess
import sys
import time
import unittest
import struct
import os
from confluo.rpc import rpc_client
from confluo.rpc import data_types
from confluo.rpc.schema import schema
from confluo.rpc.schema import schema_builder
from confluo.rpc.storage import storage_id
from thrift.transport import TTransport

class test_rpc_client(unittest.TestCase):
    SERVER_EXECUTABLE = os.getenv('CONFLUO_SERVER_EXEC', 'confluod')
        
    def wait_till_server_ready(self):
        check = True
        while check:
            try:
                c = rpc_client.rpc_client("127.0.0.1", 9090)
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

    def test_conncurrent_connections(self):
        self.start_server()
        clients = [rpc_client.rpc_client("127.0.0.1", 9090) for x in xrange(4)]
        
        try:
            for c in clients:
                c.disconnect()
        except:
            self.stop_server()
            raise

        self.stop_server()

    def test_create_atomic_multilog(self):
        self.start_server()
        client = rpc_client.rpc_client("127.0.0.1", 9090) 

        try:
            builder = schema_builder() 
            multilog_schema = schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build())
            client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.IN_MEMORY)
        except:
            self.stop_server()
            raise

        client.disconnect()
        self.stop_server()

    def test_read_write_in_memory(self):
        self.start_server()
        client = rpc_client.rpc_client("127.0.0.1", 9090) 

        try:
            builder = schema_builder() 
            multilog_schema = schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build())
            client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.IN_MEMORY)

            client.write(struct.pack("l", self.now_ns()) + "abcdefgh")
            buf = client.read(0)
            self.assertTrue(buf[8:] == "abcdefgh")
        except:
            self.stop_server()
            raise
        
        client.disconnect()
        self.stop_server()

    def test_read_write_durable_relaxed(self):
        self.start_server()
        client = rpc_client.rpc_client("127.0.0.1", 9090) 

        try:
            builder = schema_builder() 
            multilog_schema = schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build())
            client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.DURABLE_RELAXED)

            client.write(struct.pack("l", self.now_ns()) + "abcdefgh")
            buf = client.read(0)
            self.assertTrue(buf[8:] == "abcdefgh")
        except:
            self.stop_server()
            raise
        
        client.disconnect()
        self.stop_server()

    def test_read_write_durable(self):
        self.start_server()
        client = rpc_client.rpc_client("127.0.0.1", 9090) 

        try:
            builder = schema_builder() 
            multilog_schema = schema(builder.add_column(data_types.STRING_TYPE(8), "msg").build())
            client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.DURABLE)

            client.write(struct.pack("l", self.now_ns()) + "abcdefgh")
            buf = client.read(0)
            self.assertTrue(buf[8:] == "abcdefgh")
        except:
            self.stop_server()
            raise
        
        client.disconnect()
        self.stop_server()

    def test_execute_filter(self):

        self.start_server()
        client = rpc_client.rpc_client("127.0.0.1", 9090) 
    
        try:
            multilog_schema = schema(self.build_schema())
            client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.IN_MEMORY)

            client.add_index("a", 1)
            client.add_index("b", 1)
            client.add_index("c", 10)
            client.add_index("d", 2)
            client.add_index("e", 100)
            client.add_index("f", 0.1)
            client.add_index("g", 0.01)
            client.add_index("h", 1)
            
            client.write(self.pack_record(False, "0", 0, 0, 0, 0.0, 0.01, "abc"))
            client.write(self.pack_record(True, "1", 10, 2, 1, 0.1, 0.02, "defg"))
            client.write(self.pack_record(False, "2", 20, 4, 10, 0.2, 0.03, "hijkl"))
            client.write(self.pack_record(True, "3", 30, 6, 100, 0.3, 0.04, "mnopqr"))
            client.write(self.pack_record(False, "4", 40, 8, 1000, 0.4, 0.05, "stuvwx"))
            client.write(self.pack_record(True, "5", 50, 10, 10000, 0.5, 0.06, "yyy"))
            client.write(self.pack_record(False, "6", 60, 12, 100000, 0.6, 0.07, "zzz"))
            client.write(self.pack_record(True, "7", 70, 14, 1000000, 0.7, 0.08, "zzz"))

            i = 0
            for record in client.execute_filter("a == true"):
                self.assertTrue(record.at(1).unpack() == True)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.execute_filter("b > 4"):
                self.assertTrue(record.at(2).unpack() > 4)
                i += 1
            self.assertTrue(i == 3)

            i = 0
            for record in client.execute_filter("c <= 30"):
                self.assertTrue(record.at(3).unpack() <= 30)
                i += 1
            self.assertTrue(i == 4)
            
            i = 0
            for record in client.execute_filter("d == 0"):
                self.assertTrue(record.at(4).unpack() == 0)
                i += 1
            self.assertTrue(i == 1)
            
            i = 0
            for record in client.execute_filter("e <= 100"):
                self.assertTrue(record.at(5).unpack() <= 100)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.execute_filter("f > 0.1"):
                self.assertTrue(record.at(6).unpack() > 0.1)
                i += 1
            self.assertTrue(i == 6)

            i = 0
            for record in client.execute_filter("g < 0.06"):
                self.assertTrue(record.at(7).unpack() < 0.06)
                i += 1
            self.assertTrue(i == 5)

            i = 0
            for record in client.execute_filter("h == zzz"):
                self.assertTrue(record.at(8).unpack()[:3] == "zzz")
                i += 1
            self.assertTrue(i == 2)

            i = 0
            for record in client.execute_filter("a == true && b > 4"):
                self.assertTrue(record.at(1).unpack() == True)
                self.assertTrue(record.at(2).unpack() > 4)
                i += 1
            self.assertTrue(i == 2)

            i = 0
            for record in client.execute_filter("a == true && (b > 4 || c <= 30)"):
                self.assertTrue(record.at(1).unpack() == True)
                self.assertTrue(record.at(2).unpack() > 4 or record.at(3).unpack() <= 30)
                i += 1
            self.assertTrue(i == 4)
            
            i = 0
            for record in client.execute_filter("a == true && (b > 4 || f > 0.1)"):
                self.assertTrue(record.at(1).unpack() == True)
                self.assertTrue(record.at(2).unpack() > 4 or record.at(6).unpack() > 0.1)
                i += 1
            self.assertTrue(i == 3)
        except:
            self.stop_server()
            raise

        client.disconnect()
        self.stop_server()

    def test_query_filter(self):

        self.start_server()
        client = rpc_client.rpc_client("127.0.0.1", 9090) 
    
        try:
            multilog_schema = schema(self.build_schema())
            client.create_atomic_multilog("my_multilog", multilog_schema, storage_id.IN_MEMORY)

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
            client.write(self.pack_record_time(now, False, "0", 0, 0, 0, 0.0, 0.01, "abc"))
            client.write(self.pack_record_time(now, True, "1", 10, 2, 1, 0.1, 0.02, "defg"))
            client.write(self.pack_record_time(now, False, "2", 20, 4, 10, 0.2, 0.03, "hijkl"))
            client.write(self.pack_record_time(now, True, "3", 30, 6, 100, 0.3, 0.04, "mnopqr"))
            client.write(self.pack_record_time(now, False, "4", 40, 8, 1000, 0.4, 0.05, "stuvwx"))
            client.write(self.pack_record_time(now, True, "5", 50, 10, 10000, 0.5, 0.06, "yyy"))
            client.write(self.pack_record_time(now, False, "6", 60, 12, 100000, 0.6, 0.07, "zzz"))
            client.write(self.pack_record_time(now, True, "7", 70, 14, 1000000, 0.7, 0.08, "zzz"))

            i = 0
            for record in client.query_filter("filter1", beg_ms, end_ms):
                self.assertTrue(record.at(1).unpack() == True)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.query_filter("filter2", beg_ms, end_ms):
                self.assertTrue(record.at(2).unpack() > 4)
                i += 1
            self.assertTrue(i == 3)

            i = 0
            for record in client.query_filter("filter3", beg_ms, end_ms):
                self.assertTrue(record.at(3).unpack() <= 30)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.query_filter("filter4", beg_ms, end_ms):
                self.assertTrue(record.at(4).unpack() == 0)
                i += 1
            self.assertTrue(i == 1)

            i = 0
            for record in client.query_filter("filter5", beg_ms, end_ms):
                self.assertTrue(record.at(5).unpack() <= 100)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.query_filter("filter6", beg_ms, end_ms):
                self.assertTrue(record.at(6).unpack() > 0.1)
                i += 1
            self.assertTrue(i == 6)

            i = 0
            for record in client.query_filter("filter7", beg_ms, end_ms):
                self.assertTrue(record.at(7).unpack() < 0.06)
                i += 1
            self.assertTrue(i == 5)

            i = 0
            for record in client.query_filter("filter8", beg_ms, end_ms):
                self.assertTrue(record.at(8).unpack()[:3] == "zzz")
                i += 1
            self.assertTrue(i == 2)

            i = 0
            for record in client.query_filter("filter1", beg_ms, end_ms, "b > 4 || c <= 30"):
                self.assertTrue(record.at(1).unpack() == True)
                self.assertTrue(record.at(2).unpack() > 4 or record.at(3).unpack() <= 30)
                i += 1
            self.assertTrue(i == 4)

            i = 0
            for record in client.query_filter("filter1", beg_ms, end_ms, "b > 4 || f > 0.1"):
                self.assertTrue(record.at(1).unpack() == True)
                self.assertTrue(record.at(2).unpack() > 4 or record.at(6).unpack() > 0.1)
                i += 1
            self.assertTrue(i == 3)
            
            val1 = client.get_aggregate("agg1", beg_ms, end_ms)
            self.assertTrue("int(32)" == val1)
            
            val2 = client.get_aggregate("agg2", beg_ms, end_ms)
            self.assertTrue("int(36)" == val2)
            
            val3 = client.get_aggregate("agg3", beg_ms, end_ms)
            self.assertTrue("int(12)" == val3)
            
            val4 = client.get_aggregate("agg4", beg_ms, end_ms)
            self.assertTrue("int(0)" == val4)
            
            val5 = client.get_aggregate("agg5", beg_ms, end_ms)
            self.assertTrue("int(12)" == val5)
            
            val6 = client.get_aggregate("agg6", beg_ms, end_ms)
            self.assertTrue("int(54)" == val6)
            
            val7 = client.get_aggregate("agg7", beg_ms, end_ms)
            self.assertTrue("int(20)" == val7)
            
            val8 = client.get_aggregate("agg8", beg_ms, end_ms)
            self.assertTrue("int(26)" == val8)
            
        except:
            self.stop_server()
            raise

        client.disconnect()
        self.stop_server()

    def build_schema(self):
        builder = schema_builder()
        builder.add_column(data_types.BOOL_TYPE, "a")
        builder.add_column(data_types.CHAR_TYPE, "b")
        builder.add_column(data_types.SHORT_TYPE, "c")
        builder.add_column(data_types.INT_TYPE, "d")
        builder.add_column(data_types.LONG_TYPE, "e")
        builder.add_column(data_types.FLOAT_TYPE, "f")
        builder.add_column(data_types.DOUBLE_TYPE, "g")
        builder.add_column(data_types.STRING_TYPE(16), "h")
        return builder.build()

    def pack_record(self, a, b, c, d, e, f, g, h):
        rec = ""
        rec += struct.pack("l", self.now_ns())
        rec += struct.pack("?", a)
        rec += struct.pack("c", b)
        rec += struct.pack("h", c)
        rec += struct.pack("i", d)
        rec += struct.pack("l", e)
        rec += struct.pack("f", f)
        rec += struct.pack("d", g)
        rec += struct.pack("16s", h)
        return rec
    
    def pack_record_time(self, ts, a, b, c, d, e, f, g, h):
        rec = ""
        rec += struct.pack("l", ts)
        rec += struct.pack("?", a)
        rec += struct.pack("c", b)
        rec += struct.pack("h", c)
        rec += struct.pack("i", d)
        rec += struct.pack("l", e)
        rec += struct.pack("f", f)
        rec += struct.pack("d", g)
        rec += struct.pack("16s", h)
        return rec

    def now_ns(self):
        return int(time.time() * 10**6)

    def time_block(self, ts):
        return ts / 10**6

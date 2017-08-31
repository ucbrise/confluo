#!/usr/bin/env python3
import env
import subprocess
import sys
import time
import unittest
import struct
from dialog import dialog_client
from dialog.data_types import *
from dialog.schema import schema
from dialog.schema import schema_builder
from dialog.storage import storage_id

class test_dialog_client(unittest.TestCase):

    SERVER_EXECUTABLE = "dialogd"

    def start_server(self):
        self.server = subprocess.Popen([self.SERVER_EXECUTABLE])
        time.sleep(1)

    def stop_server(self):
        self.server.kill()
        time.sleep(1)

    def test_conncurrent_connections(self):
        self.start_server()
        clients = [dialog_client.dialog_client("127.0.0.1", 9090) for x in xrange(4)]

        for c in clients:
            c.disconnect()

        self.stop_server()

    def test_create_table(self):
        self.start_server()
        client = dialog_client.dialog_client("127.0.0.1", 9090) 

        builder = schema_builder() 
        table_schema = schema("/tmp", builder.add_column(STRING_TYPE(8), "msg").build())
        client.create_table("table", table_schema, storage_id.IN_MEMORY)

        client.disconnect()
        self.stop_server()

    def test_read_write_in_memory(self):
        self.start_server()
        client = dialog_client.dialog_client("127.0.0.1", 9090) 

        builder = schema_builder() 
        table_schema = schema("/tmp", builder.add_column(STRING_TYPE(8), "msg").build())
        client.create_table("my_table", table_schema, storage_id.IN_MEMORY)

        client.write(struct.pack("l", self.now_ns()) + "abcdefgh")
        buf = client.read(0)
        assert buf[8:] == "abcdefgh"
        
        client.disconnect()
        self.stop_server()

    def test_read_write_durable_relaxed(self):
        self.start_server()
        client = dialog_client.dialog_client("127.0.0.1", 9090) 

        builder = schema_builder() 
        table_schema = schema("/tmp", builder.add_column(STRING_TYPE(8), "msg").build())
        client.create_table("my_table", table_schema, storage_id.DURABLE_RELAXED)

        client.write(struct.pack("l", self.now_ns()) + "abcdefgh")
        buf = client.read(0)
        assert buf[8:] == "abcdefgh"
        
        client.disconnect()
        self.stop_server()

    def test_read_write_durable(self):
        self.start_server()
        client = dialog_client.dialog_client("127.0.0.1", 9090) 

        builder = schema_builder() 
        table_schema = schema("/tmp", builder.add_column(STRING_TYPE(8), "msg").build())
        client.create_table("my_table", table_schema, storage_id.DURABLE)

        client.write(struct.pack("l", self.now_ns()) + "abcdefgh")
        buf = client.read(0)
        assert buf[8:] == "abcdefgh"
        
        client.disconnect()
        self.stop_server()

    def test_read_buffer(self):
        self.start_server()
        client = dialog_client.dialog_client("127.0.0.1", 9090) 

        builder = schema_builder() 
        table_schema = schema("/tmp", builder.add_column(STRING_TYPE(8), "msg").build())
        client.create_table("my_table", table_schema, storage_id.IN_MEMORY)

        client.buffer(struct.pack("l", self.now_ns()) + "abcdefgh")
        client.buffer(struct.pack("l", self.now_ns()) + "ijklmnop")
        client.buffer(struct.pack("l", self.now_ns()) + "qrstuvwx")
        client.flush()

        buf = client.read(0)
        assert buf[8:] == "abcdefgh"
        buf = client.read(table_schema.record_size_)
        assert buf[8:] == "ijklmnop"
        buf = client.read(table_schema.record_size_ * 2)
        assert buf[8:] == "qrstuvwx"
        
        client.disconnect()
        self.stop_server()

    def test_adhoc_filter(self):

        self.start_server()
        client = dialog_client.dialog_client("127.0.0.1", 9090) 
    
        table_schema = schema("/tmp", self.build_schema())
        client.create_table("my_table", table_schema, storage_id.IN_MEMORY)

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
        for record in client.adhoc_filter("a == true"):
            assert record.at(1).unpack() == True
            i += 1
        assert i == 4

        i = 0
        for record in client.adhoc_filter("b > 4"):
            assert record.at(2).unpack() > 4
            i += 1
        assert i == 3

        i = 0
        for record in client.adhoc_filter("c <= 30"):
            assert record.at(3).unpack() <= 30
            i += 1
        assert i == 4
        
        i = 0
        for record in client.adhoc_filter("d == 0"):
            assert record.at(4).unpack() == 0
            i += 1
        assert i == 1
        
        i = 0
        for record in client.adhoc_filter("e <= 100"):
            assert record.at(5).unpack() <= 100
            i += 1
        assert i == 4

        i = 0
        for record in client.adhoc_filter("f > 0.1"):
            assert record.at(6).unpack() > 0.1
            i += 1
        assert i == 6

        i = 0
        for record in client.adhoc_filter("g < 0.06"):
            assert record.at(7).unpack() < 0.06
            i += 1
        assert i == 5

        i = 0
        for record in client.adhoc_filter("h == zzz"):
            assert record.at(8).unpack()[:3] == "zzz"
            i += 1
        assert i == 2

        i = 0
        for record in client.adhoc_filter("a == true && b > 4"):
            assert record.at(1).unpack() == True
            assert record.at(2).unpack() > 4
            i += 1
        assert i == 2

        i = 0
        for record in client.adhoc_filter("a == true && (b > 4 || c <= 30)"):
            assert record.at(1).unpack() == True
            assert record.at(2).unpack() > 4 or record.at(3).unpack() <= 30
            i += 1
        assert i == 4
        
        i = 0
        for record in client.adhoc_filter("a == true && (b > 4 || f > 0.1)"):
            assert record.at(1).unpack() == True
            assert record.at(2).unpack() > 4 or record.at(6).unpack() > 0.1
            i += 1
        assert i == 3

        client.disconnect()
        self.stop_server()

    def test_predef_filter(self):

        self.start_server()
        client = dialog_client.dialog_client("127.0.0.1", 9090) 
    
        table_schema = schema("/tmp", self.build_schema())
        client.create_table("my_table", table_schema, storage_id.IN_MEMORY)

        client.add_filter("filter1", "a == true")
        client.add_filter("filter2", "b > 4")
        client.add_filter("filter3", "c <= 30")
        client.add_filter("filter4", "d == 0")
        client.add_filter("filter5", "e <= 100")
        client.add_filter("filter6", "f > 0.1")
        client.add_filter("filter7", "g < 0.06")
        client.add_filter("filter8", "h == zzz")
        client.add_trigger("trigger1", "filter1", "SUM(d) >= 10")
        client.add_trigger("trigger2", "filter2", "SUM(d) >= 10")
        client.add_trigger("trigger3", "filter3", "SUM(d) >= 10")
        client.add_trigger("trigger4", "filter4", "SUM(d) >= 10")
        client.add_trigger("trigger5", "filter5", "SUM(d) >= 10")
        client.add_trigger("trigger6", "filter6", "SUM(d) >= 10")
        client.add_trigger("trigger7", "filter7", "SUM(d) >= 10")
        client.add_trigger("trigger8", "filter8", "SUM(d) >= 10")

        beg_ms = self.time_block(self.now_ns())
        client.write(self.pack_record(False, "0", 0, 0, 0, 0.0, 0.01, "abc"))
        client.write(self.pack_record(True, "1", 10, 2, 1, 0.1, 0.02, "defg"))
        client.write(self.pack_record(False, "2", 20, 4, 10, 0.2, 0.03, "hijkl"))
        client.write(self.pack_record(True, "3", 30, 6, 100, 0.3, 0.04, "mnopqr"))
        client.write(self.pack_record(False, "4", 40, 8, 1000, 0.4, 0.05, "stuvwx"))
        client.write(self.pack_record(True, "5", 50, 10, 10000, 0.5, 0.06, "yyy"))
        client.write(self.pack_record(False, "6", 60, 12, 100000, 0.6, 0.07, "zzz"))
        client.write(self.pack_record(True, "7", 70, 14, 1000000, 0.7, 0.08, "zzz"))
        end_ms = self.time_block(self.now_ns())

        i = 0
        for record in client.predef_filter("filter1", beg_ms, end_ms):
            assert record.at(1).unpack() == True
            i += 1
        assert i == 4

        i = 0
        for record in client.predef_filter("filter2", beg_ms, end_ms):
            assert record.at(2).unpack() > 4
            i += 1
        assert i == 3

        i = 0
        for record in client.predef_filter("filter3", beg_ms, end_ms):
            assert record.at(3).unpack() <= 30
            i += 1
        assert i == 4

        i = 0
        for record in client.predef_filter("filter4", beg_ms, end_ms):
            assert record.at(4).unpack() == 0
            i += 1
        assert i == 1

        i = 0
        for record in client.predef_filter("filter5", beg_ms, end_ms):
            assert record.at(5).unpack() <= 100
            i += 1
        assert i == 4

        i = 0
        for record in client.predef_filter("filter6", beg_ms, end_ms):
            assert record.at(6).unpack() > 0.1
            i += 1
        assert i == 6

        i = 0
        for record in client.predef_filter("filter7", beg_ms, end_ms):
            assert record.at(7).unpack() < 0.06
            i += 1
        assert i == 5

        i = 0
        for record in client.predef_filter("filter8", beg_ms, end_ms):
            assert record.at(8).unpack()[:3] == "zzz"
            i += 1
        assert i == 2

        i = 0
        for record in client.combined_filter("filter1", "b > 4 || c <= 30", beg_ms, end_ms):
            assert record.at(1).unpack() == True
            assert record.at(2).unpack() > 4 or record.at(3).unpack() <= 30
            i += 1
        assert i == 4

        i = 0
        for record in client.combined_filter("filter1", "b > 4 || f > 0.1", beg_ms, end_ms):
            assert record.at(1).unpack() == True
            assert record.at(2).unpack() > 4 or record.at(6).unpack() > 0.1
            i += 1
        assert i == 3

        client.disconnect()
        self.stop_server()

    def build_schema(self):
        builder = schema_builder()
        builder.add_column(BOOL_TYPE, "a")
        builder.add_column(CHAR_TYPE, "b")
        builder.add_column(SHORT_TYPE, "c")
        builder.add_column(INT_TYPE, "d")
        builder.add_column(LONG_TYPE, "e")
        builder.add_column(FLOAT_TYPE, "f")
        builder.add_column(DOUBLE_TYPE, "g")
        builder.add_column(STRING_TYPE(16), "h")
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

    def now_ns(self):
        return int(time.time() * 10**6)

    def time_block(self, ts):
        return ts / 10**6

if __name__ == '__main__':
    if len(sys.argv) == 2:
        test_dialog_client.SERVER_EXECUTABLE = sys.argv.pop()
    else:
        raise ValueError("Must provide server executable")
    unittest.main()

from collections import defaultdict
from dialog.ttypes import rpc_record_batch, rpc_record_block
import struct

class rpc_record_batch_builder:

    TIME_BLOCK = 1e6
    
    def __init__(self):
        self.num_records_ = 0
        self.clear()

    def add_record(self, record):
        ts = struct.unpack('l', record[:8])[0]
        time_block = int(ts // self.TIME_BLOCK)
        self.batch_sizes_[time_block] = len(record) + self.batch_sizes_.get(time_block, 0)
        self.batch_[time_block].append(record)
        self.num_records_ += 1

    def get_batch(self):
        batch = rpc_record_batch([], self.num_records_)
        for time_block in self.batch_:
            data = "".join(self.batch_[time_block])
            num_records = len(self.batch_[time_block])
            batch.blocks.append(rpc_record_block(time_block, data, num_records))
        self.clear()
        return batch

    def clear(self):
        self.batch_ = defaultdict(list)
        self.batch_sizes_ = {}

import struct
from collections import defaultdict

from ttypes import rpc_record_batch, rpc_record_block


class RecordBatchBuilder:
    """ A builder for a batch of records.
    """
    TIME_BLOCK = 1e6

    def __init__(self, schema):
        """ Initializes an empty rpc record batch builder.
        """
        self.schema_ = schema
        self.num_records_ = 0
        self.batch_ = defaultdict(list)
        self.batch_sizes_ = {}

    def add_record(self, record):
        """ Adds a record to the batch builder.

        Args:
            record: The record to add to the batch builder.
        """
        buf = self.schema_.pack(record)
        ts = struct.unpack('l', buf[:8])[0]
        time_block = int(ts / self.TIME_BLOCK)
        self.batch_sizes_[time_block] = len(buf) + self.batch_sizes_.get(time_block, 0)
        self.batch_[time_block].append(buf)
        self.num_records_ += 1

    def get_batch(self):
        """ Gets the record batch.

        Returns:
            The record batch containing the records.
        """
        batch = rpc_record_batch([], self.num_records_)
        for time_block in self.batch_:
            data = "".join(self.batch_[time_block])
            num_records = len(self.batch_[time_block])
            batch.blocks.append(rpc_record_block(time_block, data, num_records))
        self.clear()
        return batch

    def clear(self):
        """ Clears the record batch builder.
        """
        self.batch_ = defaultdict(list)
        self.batch_sizes_ = {}

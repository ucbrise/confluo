import data_types
from data_types import type_id
import struct

class schema:

    def __init__(self, columns):
        self.record_size_ = 0
        self.columns_ = columns
        for column in self.columns_:
            self.record_size_ += column.data_type_.size_

    def apply(self, offset, data):
        rec = record(offset, data, self.record_size_)
        for column in self.columns_:
            rec.push_back(column.apply(rec.data_))
        return rec

class column:
    
    def __init__(self, idx, offset, dtype, name, min_value, max_value):
        self.idx_ = idx
        self.offset_ = offset
        self.data_type_ = dtype
        self.name_ = name.upper()
        self.min_value_ = min_value
        self.max_value = max_value

    def apply(self, data):
        return field(self.idx_, self.data_type_, data[self.offset_ : self.offset_ + self.data_type_.size_])

class record:

    def __init__(self, log_offset, data, size):
        self.log_offset_ = log_offset
        self.data_ = data
        self.size_ = size
        self.version_ = self.log_offset_ + self.size_
        self.fields_ = []

    def push_back(self, value):
        self.fields_.append(value)

    def at(self, idx):
        return self.fields_[idx]

class field:

    FORMAT_CODES = { 
        type_id.D_BOOL: '?',
        type_id.D_CHAR: 'c',
        type_id.D_SHORT: 'h',
        type_id.D_INT: 'i',
        type_id.D_LONG: 'l',
        type_id.D_FLOAT: 'f',
        type_id.D_DOUBLE: 'd',
        type_id.D_STRING: 's'
    }

    def __init__(self, idx, dtype, data):
        self.idx_ = idx
        self.data_type_ = dtype
        self.data_ = data

    def unpack(self):
        tid = self.data_type_.type_id_
        if tid == type_id.D_STRING:
            format_code = str(self.data_type_.size_) + self.FORMAT_CODES[tid]
        else:
            format_code = self.FORMAT_CODES[tid]
        return struct.unpack(format_code, self.data_)[0]

class schema_builder:

    def __init__(self):
        self.user_provided_ts_ = False
        self.offset_ = 0
        self.columns_ = []
        timestamp_col = column(0, 0, data_types.LONG_TYPE, "TIMESTAMP", None, None)
        self.columns_.append(timestamp_col)
        self.offset_ += data_types.LONG_TYPE.size_

    def add_column(self, dtype, name, min_value=None, max_value=None):
        if name.upper() == "TIMESTAMP":
            self.user_provided_ts_ = True
            if dtype != data_types.LONG_TYPE:
                raise ValueError("TIMESTAMP must be of LONG_TYPE")
            return self
        col = column(len(self.columns_), self.offset_, dtype, name, min_value, max_value)
        self.columns_.append(col)
        self.offset_ += dtype.size_
        return self

    def build(self):
        return self.columns_


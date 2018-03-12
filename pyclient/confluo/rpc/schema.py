import data_types
from data_types import type_id
import struct

class schema:
    """ The schema for the data in the atomic multilog.
    """
    def __init__(self, columns):
        """ Initializes the schema to the list of columns passed in.
        Args:
            columns: The list of columns that make up the schema.
        """
        self.record_size_ = 0
        self.columns_ = columns
        for column in self.columns_:
            self.record_size_ += column.data_type_.size_

    def apply(self, offset, data):
        """ Adds data to the schema.
        Args:
            offset: The offset in the record where the data is
            data: The data to add.
        Returns:
            The record.
        """
        rec = record(offset, data, self.record_size_)
        for column in self.columns_:
            rec.push_back(column.apply(rec.data_))
        return rec

class column:
    """ Container of values for a specific type in the schema.
    """
    def __init__(self, idx, offset, dtype, name, min_value, max_value):
        """ Initializes a column in the schema.
        Args:
            idx: The index of the column.
            offset: The offset of the column.
            dtype: The data type of values in the column.
            name: The name of the column.
            min_value: The minimum value of the column.
            max_value: The maximum value of the column.
        """
        self.idx_ = idx
        self.offset_ = offset
        self.data_type_ = dtype
        self.name_ = name.upper()
        self.min_value_ = min_value
        self.max_value = max_value

    """ Adds data to the column.
    Args:
        data: The data to add.
    Returns:
        A field containing the data.
    """
    def apply(self, data):
        return field(self.idx_, self.data_type_, data[self.offset_ : self.offset_ + self.data_type_.size_])

class record:
    """ A collection of values containing different types.
    """
    def __init__(self, log_offset, data, size):
        """
        Initializes a record to the specified values.
        Args:
            log_offset: The offset from the log.
            data: The data the record should hold.
            size: The size of the record in bytes.
        """
        self.log_offset_ = log_offset
        self.data_ = data
        self.size_ = size
        self.version_ = self.log_offset_ + self.size_
        self.fields_ = []

    def push_back(self, value):
        """ Adds a value to the record.
        Args:
            value: The value to add.
        """
        self.fields_.append(value)

    def at(self, idx):
        """ Gets a field at a specific index.
        Args:
            The index of the desired field.
        Returns:
            The desired field.
        """
        return self.fields_[idx]

class field:
    """ Contains data stored as part of a record.
    Attributes:
        FORMAT_CODES: Identifiers for different data types.
    """
    FORMAT_CODES = { 
        type_id.BOOL: '?',
        type_id.CHAR: 'c',
        type_id.SHORT: 'h',
        type_id.INT: 'i',
        type_id.LONG: 'l',
        type_id.FLOAT: 'f',
        type_id.DOUBLE: 'd',
        type_id.STRING: 's'
    }

    def __init__(self, idx, dtype, data):
        """ Initializes the field to the data passed in.
        Args:
            idx: The index of the field.
            dtype: The data type the value of the field contains.
            data: The data that the field contains.
        """
        self.idx_ = idx
        self.data_type_ = dtype
        self.data_ = data

    def unpack(self):
        """ Unpacks the field to get the data.
        Returns:
            The data in the field.
        """
        tid = self.data_type_.type_id_
        if tid == type_id.STRING:
            format_code = str(self.data_type_.size_) + self.FORMAT_CODES[tid]
        else:
            format_code = self.FORMAT_CODES[tid]
        return struct.unpack(format_code, self.data_)[0]

class schema_builder:

    """ Builder of a schema for the atomic multilog.
    """
    def __init__(self):
        """ Initializes a default schema builder.
        """
        self.user_provided_ts_ = False
        self.offset_ = 0
        self.columns_ = []
        timestamp_col = column(0, 0, data_types.ULONG_TYPE, "TIMESTAMP", None, None)
        self.columns_.append(timestamp_col)
        self.offset_ += data_types.ULONG_TYPE.size_

    def add_column(self, dtype, name, min_value=None, max_value=None):
        """ Adds a column to the schema builder.

        Args:
            dtype: The data type of the column.
            name: The name of the column.
            min_value: The minimum value of the column.
            max_value: The maximum value of the column.
        """
        if name.upper() == "TIMESTAMP":
            self.user_provided_ts_ = True
            if dtype != data_types.ULONG_TYPE:
                raise ValueError("TIMESTAMP must be of ULONG_TYPE")
            return self
        col = column(len(self.columns_), self.offset_, dtype, name, min_value, max_value)
        self.columns_.append(col)
        self.offset_ += dtype.size_
        return self

    def build(self):
        """ Builds a schema by returning the list of columns.

        Returns:
            A list of columns that make up the schema.
        """
        return self.columns_


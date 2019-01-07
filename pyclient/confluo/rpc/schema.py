import struct
import time
import yaml
import yaml.resolver
from collections import OrderedDict

import data_types
from data_types import TypeID


def now_ns():
    return int(time.time() * 10 ** 6)


class Schema:
    """ The schema for the data in the atomic multilog.
    """

    def __init__(self, columns):
        """ Initializes the schema to the list of columns passed in.

        Args:
            columns: The list of columns that make up the schema.
        """
        self.record_size_ = 0
        self.columns_ = columns
        for c in self.columns_:
            self.record_size_ += c.data_type_.size_

    def __str__(self):
        """ Convert to string

        Returns:
            String representation of schema
        """
        return str(self.columns_)

    def record_size(self):
        """ Get record size in bytes

        Returns:
            Record size in bytes
        """
        return self.record_size_

    def columns(self):
        """ Get list of columns

        Returns:
            List of columns
        """
        return self.columns_

    def apply(self, data):
        """ Adds data to the schema.

        Args:
            data: The data to add.
        Returns:
            The record.
        """
        return Record(data, self)

    def pack(self, rec):
        """ Pack data into a record.

        Args:
            rec: The record to pack
        Returns:
            Packed record
        """
        packed = ""
        if len(rec) == len(self.columns_):
            off = 1
            packed += struct.pack('Q', rec[0])
        elif len(rec) == len(self.columns_) - 1:
            off = 0
            packed += struct.pack('Q', now_ns())
        else:
            raise ValueError("Record does not conform to schema: incorrect number of fields")

        for f, c in zip(rec[off:], self.columns_[1:]):
            packed += c.data_type_.pack(f)
        return packed


class Column:
    """ Container of values for a specific type in the schema.
    """

    def __init__(self, idx, offset, data_type, name, min_value, max_value):
        """ Initializes a column in the schema.

        Args:
            idx: The index of the column.
            offset: The offset of the column.
            data_type: The data type of values in the column.
            name: The name of the column.
            min_value: The minimum value of the column.
            max_value: The maximum value of the column.
        """
        self.idx_ = idx
        self.offset_ = offset
        self.data_type_ = data_type
        self.name_ = name.upper()
        self.value = min_value
        self.min_value_ = self.value
        self.max_value = max_value

    def __str__(self):
        """ Convert to string

        Returns:
            String representation of the column
        """
        return '{} : {}'.format(self.name_, self.data_type_)

    def apply(self, data):
        """ Adds data to the column.

        Args:
            data: The data to add.
        Returns:
            A field containing the data.
        """

        return Field(self.idx_, self.data_type_, data[self.offset_: self.offset_ + self.data_type_.size_])


class Record:
    """ A collection of values containing different types.
    """

    def __init__(self, data, schema):
        """
        Initializes a record to the specified values.

        Args:
            data: The data the record should hold.
            schema: The schema for the record.
        """
        self.data_ = data
        self.fields_ = [c.apply(self.data_) for c in schema.columns()]

    def __str__(self):
        """ Converts to string

        Returns:
            String representation of record
        """
        return str([str(x.unpack()) for x in self.fields_])

    def __getitem__(self, idx):
        """ Get element at specified index

        Args:
            idx: Index into record
        Returns:
            Element at specified index
        """
        return self.fields_[idx].unpack()


class Field:
    """ Contains data stored as part of a record.
    """

    def __init__(self, idx, data_type, data):
        """ Initializes the field to the data passed in.

        Args:
            idx: The index of the field.
            data_type: The data type the value of the field contains.
            data: The data that the field contains.
        """
        self.idx_ = idx
        self.data_type_ = data_type
        self.data_ = data

    def unpack(self):
        """ Unpacks the field to get the data.

        Returns:
            The data in the field.
        """
        tid = self.data_type_.type_id_
        if tid == TypeID.STRING:
            format_code = str(self.data_type_.size_) + data_types.FORMAT_CODES[tid]
        else:
            format_code = data_types.FORMAT_CODES[tid]
        return struct.unpack(format_code, self.data_)[0]


class SchemaBuilder:
    """ Builder of a schema for the atomic multilog.
    """

    def __init__(self):
        """ Initializes a default schema builder.
        """
        self.user_provided_ts_ = False
        self.offset_ = 0
        self.columns_ = []
        timestamp_col = Column(0, 0, data_types.ULONG_TYPE, "TIMESTAMP", None, None)
        self.columns_.append(timestamp_col)
        self.offset_ += data_types.ULONG_TYPE.size_

    def add_column(self, data_type, name, min_value=None, max_value=None):
        """ Adds a column to the schema builder.

        Args:
            data_type: The data type of the column.
            name: The name of the column.
            min_value: The minimum value of the column.
            max_value: The maximum value of the column.
        """
        if name.upper() == "TIMESTAMP":
            self.user_provided_ts_ = True
            if data_type != data_types.ULONG_TYPE:
                raise ValueError("TIMESTAMP must be of ULONG_TYPE")
            return self
        col = Column(len(self.columns_), self.offset_, data_type, name, min_value, max_value)
        self.columns_.append(col)
        self.offset_ += data_type.size_
        return self

    def build(self):
        """ Builds a schema by returning the list of columns.

        Returns:
            A list of columns that make up the schema.
        """
        return self.columns_


def make_schema(s):
    """Converts a JSON-like string representation of the schema to our internal representation of the schema.

    Args:
        s: A JSON-like schema string

    Returns:
        Our internal representation of the schema.
    """
    def ordered_load(stream):
        class OrderedLoader(yaml.Loader):
            pass

        def construct_mapping(loader, node):
            loader.flatten_mapping(node)
            return OrderedDict(loader.construct_pairs(node))

        OrderedLoader.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, construct_mapping)
        return yaml.load(stream, OrderedLoader)
    s_parsed = ordered_load(s)
    sb = SchemaBuilder()
    for k in s_parsed:
        sb.add_column(data_types.make_type(s_parsed[k]), k)
    return Schema(sb.build())

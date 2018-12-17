import re
import struct

from ttypes import rpc_data_type


class TypeID:
    """ Contains the primitive types.
    """

    def __init__(self):
        pass

    NONE = rpc_data_type.RPC_NONE
    BOOL = rpc_data_type.RPC_BOOL
    CHAR = rpc_data_type.RPC_CHAR
    UCHAR = rpc_data_type.RPC_UCHAR
    SHORT = rpc_data_type.RPC_SHORT
    USHORT = rpc_data_type.RPC_USHORT
    INT = rpc_data_type.RPC_INT
    UINT = rpc_data_type.RPC_UINT
    LONG = rpc_data_type.RPC_LONG
    ULONG = rpc_data_type.RPC_ULONG
    FLOAT = rpc_data_type.RPC_FLOAT
    DOUBLE = rpc_data_type.RPC_DOUBLE
    STRING = rpc_data_type.RPC_STRING


FORMAT_CODES = {
    TypeID.BOOL: '?',
    TypeID.CHAR: 'c',
    TypeID.UCHAR: 'B',
    TypeID.SHORT: 'h',
    TypeID.USHORT: 'H',
    TypeID.INT: 'i',
    TypeID.UINT: 'I',
    TypeID.LONG: 'q',
    TypeID.ULONG: 'Q',
    TypeID.FLOAT: 'f',
    TypeID.DOUBLE: 'd',
    TypeID.STRING: 's'
}


def to_string(tid, size):
    if tid == TypeID.NONE:
        return 'NONE'
    elif tid == TypeID.BOOL:
        return 'BOOL'
    elif tid == TypeID.CHAR:
        return 'CHAR'
    elif tid == TypeID.UCHAR:
        return 'UCHAR'
    elif tid == TypeID.SHORT:
        return 'SHORT'
    elif tid == TypeID.USHORT:
        return 'USHORT'
    elif tid == TypeID.INT:
        return 'INT'
    elif tid == TypeID.UINT:
        return 'UINT'
    elif tid == TypeID.LONG:
        return 'LONG'
    elif tid == TypeID.ULONG:
        return 'ULONG'
    elif tid == TypeID.FLOAT:
        return 'FLOAT'
    elif tid == TypeID.DOUBLE:
        return 'DOUBLE'
    elif tid == TypeID.STRING:
        return 'STRING({})'.format(size)


class DataType:
    """ Functionality for data types.

    """

    def __init__(self, typeid, size=0):
        """ Initializes a data type based on id and size.

        Args:
            typeid: The id of the data type.
            size: The size of the data type.
        """
        self.type_id_ = typeid
        self.size_ = size

    def __str__(self):
        """ Convert to string

        Returns:
            String representation of data type
        """
        return to_string(self.type_id_, self.size_)

    def __eq__(self, other):
        """ Performs an equality check between this data type and the other data type.

        Args:
            other: The other data type for comparison.
        Returns:
            True if the two data types are equal, false otherwise.
        """
        if other.type_id_ == self.type_id_ and other.size_ == self.size_:
            return True
        return False

    def __ne__(self, other):
        """ Performs a not equal comparison between two data types.

        Args:
            other: The other data type for comparison.
        Returns:
            True if the two data types are equal, false otherwise.
        """
        return not self.__eq__(other)

    def format_code(self):
        """ Get format code corresponding to data type

        Returns:
            Format code for data type
        """
        if self.type_id_ == TypeID.STRING:
            return '{}s'.format(self.size_)
        return FORMAT_CODES[self.type_id_]

    def pack(self, data):
        """

        Args:
            data: Python data
        Returns:
            Binary data
        """
        try:
            return struct.pack(self.format_code(), data)
        except Exception as e:
            raise ValueError('Error converting {} to {}: {}'.format(data, to_string(self.type_id_, self.size_), e))


BOOL_TYPE = DataType(TypeID.BOOL, 1)
CHAR_TYPE = DataType(TypeID.CHAR, 1)
UCHAR_TYPE = DataType(TypeID.UCHAR, 1)
SHORT_TYPE = DataType(TypeID.SHORT, 2)
USHORT_TYPE = DataType(TypeID.USHORT, 2)
INT_TYPE = DataType(TypeID.INT, 4)
UINT_TYPE = DataType(TypeID.UINT, 4)
LONG_TYPE = DataType(TypeID.LONG, 8)
ULONG_TYPE = DataType(TypeID.ULONG, 8)
FLOAT_TYPE = DataType(TypeID.FLOAT, 4)
DOUBLE_TYPE = DataType(TypeID.DOUBLE, 8)
STRING_TYPE = lambda size: DataType(TypeID.STRING, size)


def make_type(t_str):
    m = re.match(r'STRING\((\d+)\)', t_str)
    if t_str == 'BOOL':
        return BOOL_TYPE
    elif t_str == 'CHAR':
        return CHAR_TYPE
    elif t_str == 'UCHAR':
        return CHAR_TYPE
    elif t_str == 'SHORT':
        return SHORT_TYPE
    elif t_str == 'USHORT':
        return USHORT_TYPE
    elif t_str == 'INT':
        return INT_TYPE
    elif t_str == 'UINT':
        return UINT_TYPE
    elif t_str == 'LONG':
        return LONG_TYPE
    elif t_str == 'ULONG':
        return ULONG_TYPE
    elif t_str == 'FLOAT':
        return FLOAT_TYPE
    elif t_str == 'DOUBLE':
        return DOUBLE_TYPE
    elif m:
        return STRING_TYPE(int(m.group(1)))
    else:
        raise ValueError('Invalid type {}'.format(t_str))

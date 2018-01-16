from ttypes import rpc_data_type


class type_id:
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


class data_type:
    
    def __init__(self, typeid, size=0):
        self.type_id_ = typeid
        self.size_ = size

    def __eq__(self, other):
        if other.type_id_ == self.type_id_ and other.size_ == self.size_:
            return True 
        return False

    def __ne__(self, other):
        return not self.__eq__(other)


BOOL_TYPE = data_type(type_id.BOOL, 1)
CHAR_TYPE = data_type(type_id.CHAR, 1)
UCHAR_TYPE = data_type(type_id.UCHAR, 1)
SHORT_TYPE = data_type(type_id.SHORT, 2)
USHORT_TYPE = data_type(type_id.USHORT, 2)
INT_TYPE = data_type(type_id.INT, 4)
UINT_TYPE = data_type(type_id.UINT, 4)
LONG_TYPE = data_type(type_id.LONG, 8)
ULONG_TYPE = data_type(type_id.ULONG, 8)
FLOAT_TYPE = data_type(type_id.FLOAT, 4)
DOUBLE_TYPE = data_type(type_id.DOUBLE, 8)
STRING_TYPE = lambda size: data_type(type_id.STRING, size)
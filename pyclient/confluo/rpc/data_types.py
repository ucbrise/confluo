from ttypes import rpc_data_type

class type_id:
    D_NONE = rpc_data_type.RPC_NONE
    D_BOOL = rpc_data_type.RPC_BOOL
    D_CHAR = rpc_data_type.RPC_CHAR
    D_SHORT = rpc_data_type.RPC_SHORT
    D_INT = rpc_data_type.RPC_INT
    D_LONG = rpc_data_type.RPC_LONG
    D_FLOAT = rpc_data_type.RPC_FLOAT
    D_DOUBLE = rpc_data_type.RPC_DOUBLE
    D_STRING = rpc_data_type.RPC_STRING

class data_type:

    def __init__(self, typeid, size=0):
        self.type_id_ = typeid

        if self.type_id_ == type_id.D_NONE:
            self.size_ = 0
        elif self.type_id_ == type_id.D_BOOL: 
            self.size_ = 1
        elif self.type_id_ == type_id.D_CHAR:
            self.size_ = 1
        elif self.type_id_ == type_id.D_SHORT:
            self.size_ = 2
        elif self.type_id_ == type_id.D_INT:
            self.size_ = 4
        elif self.type_id_ == type_id.D_LONG:
            self.size_ = 8
        elif self.type_id_ == type_id.D_FLOAT:
            self.size_ = 4
        elif self.type_id_ == type_id.D_DOUBLE:
            self.size_ = 8
        else:
            self.size_ = size

    def __eq__(self, other):
        if other.type_id_ == self.type_id_ and other.size_ == self.size_:
            return True 
        return False

    def __ne__(self, other):
        return not self.__eq__(other)

BOOL_TYPE = data_type(type_id.D_BOOL)
CHAR_TYPE = data_type(type_id.D_CHAR)
SHORT_TYPE = data_type(type_id.D_SHORT)
INT_TYPE = data_type(type_id.D_INT)
LONG_TYPE = data_type(type_id.D_LONG)
FLOAT_TYPE = data_type(type_id.D_FLOAT)
DOUBLE_TYPE = data_type(type_id.D_DOUBLE)
STRING_TYPE = lambda size: data_type(type_id.D_STRING, size)

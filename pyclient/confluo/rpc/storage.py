from ttypes import *

# Alias for rpc_storage_mode
class storage_id:
    """ Different storage modes for confluo.

    Attributes:
        IN_MEMORY: Data stored in memory.
        DURABLE_RELAXED: Relaxed (no linearizable guarantees).
        DURABLE: Data is persisted.
    """
    IN_MEMORY = rpc_storage_mode.RPC_IN_MEMORY
    DURABLE_RELAXED = rpc_storage_mode.RPC_DURABLE_RELAXED
    DURABLE = rpc_storage_mode.RPC_DURABLE

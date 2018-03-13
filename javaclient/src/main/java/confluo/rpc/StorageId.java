package confluo.rpc;

/**
 * Different storage modes for Confluo
 */
public class StorageId {
  /**
   * Data stored in memory
   */
  public static rpc_storage_mode IN_MEMORY = rpc_storage_mode.RPC_IN_MEMORY;
  /**
   * Relaxed (no linearizable guarantees
   */
  public static rpc_storage_mode DURABLE_RELAXED = rpc_storage_mode.RPC_DURABLE_RELAXED;
  /**
   * Data is persisted
   */
  public static rpc_storage_mode DURABLE = rpc_storage_mode.RPC_DURABLE;
}

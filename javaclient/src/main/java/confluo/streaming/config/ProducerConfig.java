package confluo.streaming.config;

public class ProducerConfig extends ClientConfig {

  /** <code>batch.size</code> */
  public static final String MESSAGE_SIZE = "message.size"; // actual message size

  public static final String MESSAGE_SIZE_DEFAULT = "64";
  public static final String ARCHIVE_MODE = "archive.mode"; //
  public static final String ARCHIVE_MODE_DEFAULT = "RPC_IN_MEMORY"; // #RPC_DURABLE,RPC_IN_MEMORY
}

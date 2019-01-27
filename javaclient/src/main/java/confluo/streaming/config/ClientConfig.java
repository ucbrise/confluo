package confluo.streaming.config;

public class ClientConfig {

    /** <code>bootstrap.address</code> */
    public static final String BOOTSTRAP_ADDRESS="bootstrap.address";

    public static final String BOOTSTRAP_ADDRESS_DEFAULT="127.0.0.1";

    /** <code>bootstrap.port</code> */
    public static final String BOOTSTRAP_PORT="bootstrap.port";

    public static final String BOOTSTRAP_PORT_DEFAULT="9090";
    /** <code>batch.size</code> */
    public static final String BATCH_SIZE="batch.size";

    public static final String BATCH_SIZE_DEFAULT="1";

    /** <code>topic</code> */
    public static final String TOPIC="topic";   // same as multilog name

    public static final String TOPIC_DEFAULT="topic";

}

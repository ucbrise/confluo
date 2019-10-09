import logging
from thrift.protocol.TBinaryProtocol import TBinaryProtocol
from thrift.transport import TTransport, TSocket

import rpc_service
import type_conversions
from batch import RecordBatchBuilder
from schema import make_schema
from stream import RecordStream, AlertStream


class RpcClient:
    """ Client for Confluo through RPC.
    """

    def __init__(self, host='localhost', port=9090):
        """ Initializes the rpc client to the specified host and port.

        Args:
            host: The host for the client.
            port: The port number to communicate through.
        """
        logging.basicConfig(level=logging.INFO)  # TODO: Read from configuration file
        self.LOG = logging.getLogger(__name__)
        self.LOG.info("Connecting to %s:%d", host, port)
        self.socket_ = TSocket.TSocket(host, port)
        self.transport_ = TTransport.TBufferedTransport(self.socket_)
        self.protocol_ = TBinaryProtocol(self.transport_)
        self.client_ = rpc_service.Client(self.protocol_)
        self.transport_.open()
        self.client_.register_handler()
        self.cur_m_id_ = -1
        self.cur_schema_ = None

    def close(self):
        """ Closes the rpc client.
        """
        self.disconnect()

    def connect(self, host, port):
        """ Connects the rpc client to the specified host and port.

        Args:
            host: The host of the client.
            port: The port number to communicate through.
        """
        self.LOG.info("Connecting to %s:%d", host, port)
        self.socket_ = TSocket.TSocket(host, port)
        self.transport_ = TTransport.TBufferedTransport(self.socket_)
        self.protocol_ = TBinaryProtocol(self.transport_)
        self.client_ = rpc_service.Client(self.protocol_)
        self.transport_.open()
        self.client_.register_handler()

    def disconnect(self):
        """ Disconnects the rpc client from the host and port.
        """
        if self.transport_.isOpen():
            host = self.socket_.host
            port = self.socket_.port
            self.LOG.info("Disconnecting from %s:%d", host, port)
            self.client_.deregister_handler()
            self.transport_.close()

    def create_atomic_multilog(self, name, schema, storage_mode):
        """ Creates an atomic multilog for this client.

        Args:
            name: The name of the atomic multilog to create.
            schema: The schema for the atomic multilog.
            storage_mode: The mode for storage.
        """
        self.cur_schema_ = make_schema(schema)
        rpc_schema = type_conversions.convert_to_rpc_schema(self.cur_schema_)
        self.cur_m_id_ = self.client_.create_atomic_multilog(name, rpc_schema, storage_mode)

    def load_atomic_multilog(self, name):
        """ Loads the atomic multilog with specified name

        Args:
            name: The name of the atomic multilog to create.
        """
        self.cur_m_id_ = self.client_.load_atomic_multilog(name)
        self.set_current_atomic_multilog(name)

    def set_current_atomic_multilog(self, name):
        """ Sets the atomic multilog to the desired atomic multilog.

        Args:
            name: The name of atomic multilog to set the current atomic multilog to.
        """
        info = self.client_.get_atomic_multilog_info(name)
        self.cur_schema_ = type_conversions.convert_to_schema(info.schema)
        self.cur_m_id_ = info.id

    def remove_atomic_multilog(self):
        """ Removes an atomic multilog from the client.

        Raises:
            ValueError.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.remove_atomic_multilog(self.cur_m_id_)
        self.cur_m_id_ = -1

    def add_index(self, field_name, bucket_size=1):
        """ Adds an index to the atomic multilog.

        Raises:
            ValueError.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.add_index(self.cur_m_id_, field_name, bucket_size)

    def remove_index(self, field_name):
        """ Removes an index from the atomic multilog.
        
        Args:
            field_name: The name of the associated field.
        Raises:
            ValueError
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.remove_index(self.cur_m_id_, field_name)

    def add_filter(self, filter_name, filter_expr):
        """ Adds a filter to the atomic multilog.

        Args:
            filter_name: The name of the filter
            filter_expr: The filter expression
        Raises:
            ValueError
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.add_filter(self.cur_m_id_, filter_name, filter_expr)

    def remove_filter(self, filter_name):
        """ Removes a filter from the atomic multilog.

        Args:
            filter_name: The name of the filter.
        Raises:
            ValueError.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.remove_filter(self.cur_m_id_, filter_name)

    def add_aggregate(self, aggregate_name, filter_name, aggregate_expr):
        """ Adds an aggregate to the atomic multilog.

        Args:
            aggregate_name: The name of the aggregate.
            filter_name: The name of the filter.
            aggregate_expr: The aggregate expression.
        Raises:
            ValueError.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.add_aggregate(self.cur_m_id_, aggregate_name, filter_name, aggregate_expr)

    def remove_aggregate(self, aggregate_name):
        """ Removes an aggregate from the atomic multilog.

        Args:
            aggregate_name: The name of the aggregate.
        Raises:
            ValueError.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.remove_aggregate(self.cur_m_id_, aggregate_name)

    def install_trigger(self, trigger_name, trigger_expr):
        """ Adds a trigger to the atomic multilog.

        Args:
            trigger_name: The name of the trigger to add.
            trigger_expr: The trigger expression.
        Raises:
            ValueError.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.add_trigger(self.cur_m_id_, trigger_name, trigger_expr)

    def remove_trigger(self, trigger_name):
        """ Removes a trigger from the atomic multilog.

        Args:
            trigger_name: The name of the trigger.
        Raises:
            ValueError.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.remove_trigger(self.cur_m_id_, trigger_name)

    def archive(self, offset=-1):
        """ Archives the atomic multilog until provided offset.

        Args:
            offset: Offset until which multilog should be archived (-1 for full archival).
        Raises:
            ValueError.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.archive(self.cur_m_id_, offset)

    def append_raw(self, data):
        """ Append raw data to the atomic multilog.

        Args:
            data: The data to append.
        Raises:
            ValueError.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        if len(data) != self.cur_schema_.record_size_:
            raise ValueError("Record length must be: {}, is: {}".format(self.cur_schema_.record_size_, len(data)))
        return self.client_.append(self.cur_m_id_, data)

    def append(self, rec):
        """ Append record to the atomic multilog.
        Args:
            rec: The record to append.
        Raises:
            ValueError.
        """
        return self.append_raw(self.cur_schema_.pack(rec))

    def get_batch_builder(self):
        """Get a record batch builder instance

        Returns:
             A record batch builder instance.
        """
        return RecordBatchBuilder(self.cur_schema_)

    def read_raw(self, offset):
        """ Reads raw data from a specified offset.

        Args:
            offset: The offset from the log to read from.
        Raises:
            ValueError.
        Returns:
            The data at the offset.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        return self.client_.read(self.cur_m_id_, offset, self.cur_schema_.record_size_)

    def read(self, offset):
        buf = self.read_raw(offset)
        return self.cur_schema_.apply(buf)

    def get_aggregate(self, aggregate_name, begin_ms, end_ms):
        """ Gets an aggregate from the atomic multilog.

        Args:
            aggregate_name: The name of the aggregate.
            begin_ms: The beginning time in milliseconds.
            end_ms: The end time in milliseconds.
        Raises:
            ValueError.
        Returns:
            The aggregate.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        return self.client_.query_aggregate(self.cur_m_id_, aggregate_name, begin_ms, end_ms)

    def execute_filter(self, filter_expr):
        """ Executes a specified filter.

        Args:
            filter_expr: The filter expression.
        Raises:
            ValueError.
        Returns:
            Record stream containing the data.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        handle = self.client_.adhoc_filter(self.cur_m_id_, filter_expr)
        return RecordStream(self.cur_m_id_, self.cur_schema_, self.client_, handle)

    def query_filter(self, filter_name, begin_ms, end_ms, filter_expr=""):
        """ Queries a filter.

        Args:
            filter_name: The name of the filter.
            begin_ms: The beginning time in milliseconds.
            end_ms: The end time in milliseconds.
            filter_expr: The filter expression.
        Raises:
            ValueError.
        Returns:
            A record stream containing the results of the filter.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        if filter_expr == "":
            handle = self.client_.predef_filter(self.cur_m_id_, filter_name, begin_ms, end_ms)
            return RecordStream(self.cur_m_id_, self.cur_schema_, self.client_, handle)
        else:
            handle = self.client_.combined_filter(self.cur_m_id_, filter_name, filter_expr, begin_ms, end_ms)
            return RecordStream(self.cur_m_id_, self.cur_schema_, self.client_, handle)

    def get_alerts(self, begin_ms, end_ms, trigger_name=""):
        """ Gets the alerts.

        Args:
            begin_ms: The beginning time in milliseconds.
            end_ms: The end time in milliseconds.
            trigger_name: The name of the trigger.
        Raises:
            ValueError.
        Returns:
            A stream of alerts.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        if trigger_name == "":
            handle = self.client_.alerts_by_time(self.cur_m_id_, begin_ms, end_ms)
            return AlertStream(self.cur_m_id_, self.client_, handle)
        else:
            handle = self.client_.alerts_by_trigger_and_time(self.cur_m_id_, trigger_name, begin_ms, end_ms)
            return AlertStream(self.cur_m_id_, self.client_, handle)

    def num_records(self):
        """ Gets the number of records in the atomic multilog.

        Raises:
            ValueError.
        Returns:
            The number of records.
        """
        if self.cur_m_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        return self.client_.num_records(self.cur_m_id_)

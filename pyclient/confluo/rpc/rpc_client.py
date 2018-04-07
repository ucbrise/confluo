import logging
import rpc_type_conversions
import rpc_configuration_params
import rpc_service
from rpc_record_batch_builder import rpc_record_batch_builder
from rpc_stream import record_stream, alert_stream

from ttypes import *
from thrift.transport import TTransport, TSocket
from thrift.protocol.TBinaryProtocol import TBinaryProtocol

class rpc_client:
    """
    Client for Confluo through RPC
    """
    def __init__(self, host='localhost', port=9090):
        """
        Initializes the rpc client to the specified host and port
        Args:
            host: The host for the client
            port: The port number to communicate through
        """
        logging.basicConfig(level=logging.INFO)  # TODO: Read from configuration file
        self.LOG = logging.getLogger(__name__)
        self.connect(host, port)
        self.cur_multilog_id_ = -1

    def close(self):
        """
        Closes the rpc client
        """
        self.disconnect()

    def connect(self, host, port):
        """
        Connects the rpc client to the specified host and port
        Args:
            host: The host of the client
            port: The port number to communicate through
        """
        self.LOG.info("Connecting to %s:%d", host, port)
        self.socket_ = TSocket.TSocket(host, port)
        self.transport_ = TTransport.TBufferedTransport(self.socket_)
        self.protocol_ = TBinaryProtocol(self.transport_)
        self.client_ = rpc_service.Client(self.protocol_)
        self.transport_.open()
        self.client_.register_handler()

    def disconnect(self):
        """
        Disconnects the rpc client from the host and port
        """
        if self.transport_.isOpen():
            host = self.socket_.host
            port = self.socket_.port
            self.LOG.info("Disconnecting from %s:%d", host, port)
            self.client_.deregister_handler()
            self.transport_.close()

    def create_atomic_multilog(self, atomic_multilog_name, schema, storage_mode):
        """
        Creates an atomic multilog for this client
        Args:
            name: The name of the atomic multilog to create
            schema: The schema for the atomic multilog
            storage_mode: The mode for storage
        """
        self.cur_schema_ = schema
        rpc_schema = rpc_type_conversions.convert_to_rpc_schema(schema)
        self.cur_multilog_id_ = self.client_.create_atomic_multilog(atomic_multilog_name, rpc_schema, storage_mode)
        
    def set_current_atomic_multilog(self, atomic_multilog_name):
        """
        Sets the atomic multilog to the desired atomic multilog
        Args:
            atomic_multilog_name: The name of atomic multilog to set
            the current atomic multilog to
        """
        info = self.client_.get_atomic_multilog_info(atomic_multilog_name) 
        self.cur_schema_ = rpc_type_conversions.convert_to_schema(info.schema)
        self.cur_multilog_id_ = info.atomic_multilog_id
        
    def remove_atomic_multilog(self):
        """
        Removes an atomic multilog from the client
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.remove_atomic_multilog(self.cur_multilog_id_)
        self.cur_multilog_id_ = -1

    def run_command(self, json_command):
        """
        Executes a command specified by a JSON string
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.run_command(self.cur_multilog_id_, json_command)
    
    def add_index(self, field_name, bucket_size=1):
        """
        Adds an index to the atomic multilog
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.add_index(self.cur_multilog_id_, field_name, bucket_size)

    def remove_index(self, field_name):
        """
        Removes an index from the atomic multilog
        Args:
            field_name: The name of the associated field
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.remove_index(self.cur_multilog_id_, field_name)

    def add_filter(self, filter_name, filter_expr):
        """
        Adds a filter to the atomic multilog
        Args:
            filter_name: The name of the filter
            filter_expr: The filter expression
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.add_filter(self.cur_multilog_id_, filter_name, filter_expr)

    def remove_filter(self, filter_name):
        """
        Removes a filter from the atomic multilog
        Args:
            filter_name: The name of the filter
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.remove_filter(self.cur_multilog_id_, filter_name)
        
    def add_aggregate(self, aggregate_name, filter_name, aggregate_expr):
        """
        Adds an aggregate to the atomic multilog
        Args:
            aggregate_name: The name of the aggregate
            filter_name: The name of the filter
            aggregate_expr: The aggregate expression
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.add_aggregate(self.cur_multilog_id_, aggregate_name, filter_name, aggregate_expr)

    def remove_aggregate(self, aggregate_name):
        """
        Removes an aggregate from the atomic multilog
        Args:
            aggregate_name: The name of the aggregate
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.remove_aggregate(self.cur_multilog_id_, aggregate_name)

    def install_trigger(self, trigger_name, trigger_expr):
        """
        Adds a trigger to the atomic multilog
        Args:
            trigger_name: The name of the trigger to add
            trigger_expr: The trigger expression
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.add_trigger(self.cur_multilog_id_, trigger_name, trigger_expr)

    def remove_trigger(self, trigger_name):
        """
        Removes a trigger from the atomic multilog
        Args:
            trigger_name: The name of the trigger
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        self.client_.remove_trigger(self.cur_multilog_id_, trigger_name)
        
    def get_batch_builder(self):
        """
        Gets a new record batch builder
        """
        return rpc_record_batch_builder()
        
    def write(self, record):
        """
        Writes a record to the atomic multilog
        Args:
            record: The record to write
        Raises:
            ValueError
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        if len(record) != self.cur_schema_.record_size_:
            raise ValueError("Record must be of length " + str(self.cur_schema_.record_size_))
        self.client_.append(self.cur_multilog_id_, record)
            
    def read(self, offset):
        """
        Reads data from a specified offset
        Args:
            offset: The offset from the log to read from
        Raises:
            ValueError
        Returns:
            The data at the offset
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        return self.client_.read(self.cur_multilog_id_, offset, self.cur_schema_.record_size_)
    
    def get_aggregate(self, aggregate_name, begin_ms, end_ms):
        """
        Gets an aggregate from the atomic multilog
        Args:
            aggregate_name: The name of the aggregate
            begin_ms: The beginning time in milliseconds
            end_ms: The end time in milliseconds
        Raises:
            ValueError
        Returns:
            The aggregate
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        return self.client_.query_aggregate(self.cur_multilog_id_, aggregate_name, begin_ms, end_ms)

    def execute_filter(self, filter_expr):
        """
        Executes a specified filter
        Args:
            filter_expr: The filter expression
        Raises:
            ValueError
        Returns:
            Record stream containing the data
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        handle = self.client_.adhoc_filter(self.cur_multilog_id_, filter_expr)
        return record_stream(self.cur_multilog_id_, self.cur_schema_, self.client_, handle)

    def query_filter(self, filter_name, begin_ms, end_ms, filter_expr = ""):
        """
        Queries a filter
        Args:
            filter_name: The name of the filter
            begin_ms: The beginning time in milliseconds
            end_ms: The end time in milliseconds
            filter_expr: The filter expression
        Raises:
            ValueError
        Returns:
            A record stream containing the results of the filter
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        if filter_expr == "":
            handle = self.client_.predef_filter(self.cur_multilog_id_, filter_name, begin_ms, end_ms)
            return record_stream(self.cur_multilog_id_, self.cur_schema_, self.client_, handle)
        else:
            handle = self.client_.combined_filter(self.cur_multilog_id_, filter_name, filter_expr, begin_ms, end_ms)
            return record_stream(self.cur_multilog_id_, self.cur_schema_, self.client_, handle)

    def get_alerts(self, begin_ms, end_ms, trigger_name = ""):
        """
        Gets the alerts
        Args:
            begin_ms: The beginning time in milliseconds
            end_ms: The end time in milliseconds
            trigger_name: The name of the trigger
        Raises:
            ValueError
        Returns:
            A stream of alerts
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        if trigger_name == "":
            handle = self.client.alerts_by_time(self.cur_multilog_id_, handle, begin_ms, end_ms)
            return alert_stream(self.cur_multilog_id_, self.cur_schema_, self.client_, handle)
        else:
            handle = self.client.alerts_by_time(self.cur_multilog_id_, handle, trigger_name, begin_ms, end_ms)
            return alert_stream(self.cur_multilog_id_, self.cur_schema_, self.client_, handle)

    def num_records(self):
        """
        Gets the number of records in the atomic multilog
        Raises:
            ValueError
        Returns:
            The number of records
        """
        if self.cur_multilog_id_ == -1:
            raise ValueError("Must set atomic multilog first.")
        return self.client_.num_records(self.cur_multilog_id_)

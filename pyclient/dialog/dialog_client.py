import logging
from thrift.transport import TTransport, TSocket
from thrift.protocol.TBinaryProtocol import TBinaryProtocol
from dialog import dialog_service
from dialog.ttypes import *


# Add all methods from reader/writer here
class dialog_client:
    def __init__(self, host='localhost', port=9090):
        logging.basicConfig(level=logging.INFO)  # Read from configuration file
        self.LOG = logging.getLogger(__name__)
        self.connect(host, port)

    def close(self):
        self.disconnect()

    def connect(self, host, port):
        self.LOG.info("Connecting to %s:%d", host, port)
        self.socket_ = TSocket.TSocket(host, port)
        self.transport_ = TTransport.TBufferedTransport(self.socket_)
        self.protocol_ = TBinaryProtocol(self.transport_)
        self.client_ = dialog_service.Client(self.protocol_)
        self.transport_.open()
        self.client_.register_handler()

    def disconnect(self):
        if self.transport_.isOpen():
            host = self.socket_.host
            port = self.socket_.port
            self.LOG.info("Disconnecting from %s:%d", host, port)
            self.client_.deregister_handler()
            self.transport_.close()

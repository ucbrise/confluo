try:
    from cStringIO import StringIO
except:
    from StringIO import StringIO


class RecordStream:
    """ A stream of records and associated functionality.
    """

    def __init__(self, m_id, schema, client, handle):
        """ Initializes an empty record stream.

        Args:
            m_id: The identifier for the atomic multilog.
            schema: The associated schema.
            client: The rpc client.
            handle: Iterator through the stream.
        """
        self.m_id_ = m_id
        self.schema_ = schema
        self.client_ = client
        self.handle_ = handle
        self.cur_off_ = 0

    def __iter__(self):
        """ Iterator for the record stream.

        Yields:
            record: The next element in the stream.
        """
        while self.has_more():
            yield self.schema_.apply(self.handle_.data[self.cur_off_:])
            self.cur_off_ += self.schema_.record_size_
            if self.cur_off_ == len(self.handle_.data) and self.handle_.has_more:
                self.handle_ = self.client_.get_more(self.m_id_, self.handle_.desc)
                self.cur_off_ = 0

    def has_more(self):
        """ Checks whether the stream has any more elements.

        Returns:
            True if there are any more records in the stream, false otherwise
        """
        return self.handle_.has_more or self.cur_off_ != len(self.handle_.data)


class AlertStream:
    """ A stream of alerts.
    """

    def __init__(self, m_id, client, handle):
        """ Initializes a stream of alerts to the data passed in.

        Args:
            m_id: The identifier of the atomic multilog.
            client: The rpc client.
            handle: The iterator for the stream.
        """
        self.m_id_ = m_id
        self.client_ = client
        self.handle_ = handle
        self.stream_ = StringIO(handle.data)

    def __iter__(self):
        """ Iterates through the alert stream.

        Yields:
            alert: The next alert in the stream.
        """
        while self.has_more():
            alert = self.stream_.readline()
            if alert:
                yield alert
            elif self.handle_.has_more:
                self.handle_ = self.client_.get_more(self.m_id_, self.handle_.desc)
                self.stream_ = StringIO(self.handle_.data)

    def has_more(self):
        """ Checks whether the stream has any more elements.

        Returns: 
            True if there are any elements left in the stream, false otherwise.
        """
        return self.handle_.has_more or not self.stream_

class record_stream:
    """ A stream of records and associated functionality.
    """
    def __init__(self, multilog_id, schema, client, handle):
        """ Initializes an empty record stream.

        Args:
            multilog_id: The identifier for the atomic multilog.
            schema: The associated schema.
            client: The rpc client.
            handle: Iterator through the stream.
        """
        self.multilog_id_ = multilog_id
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
            yield self.schema_.apply(0, self.handle_.data[self.cur_off_:])
            self.cur_off_ += self.schema_.record_size_
            if self.cur_off_ == len(self.handle_.data) and self.handle_.has_more:
                self.handle_ = client_.get_more(self.multilog_id_, self.handle_.desc)
                self.cur_off_ = 0

    def has_more(self):
        """ Checks whether the stream has any more elements.

        Returns:
            True if there are any more records in the stream, false otherwise
        """
        return self.handle_.has_more or self.cur_off_ != len(self.handle_.data)

class alert_stream:
    """ A stream of alerts.
    """

    def __init__(self, multilog_id, client, handle):
        """ Initializes a stream of alerts to the data passed in.

        Args:
            multilog_id: The identifier of the atomic multilog.
            client: The rpc client.
            handle: The iterator for the stream.
        """
        self.multilog_id_ = multilog_id
        self.client_ = client
        self.handle_ = handle
        self.stream_ = handle.data
        
    def __iter__(self):
        """ Iterates through the alert stream.

        Yields:
            alert: The next alert in the stream.
        """
        while self.has_more():
            end_idx = alert.find("\n")
            alert = self.stream_[:end_idx]
            self.stream_ = self.stream_[end_idx:]
            yield alert
            if not alert and self.handle_.has_more: 
                self.handle_ = self.client_.get_more(self.multilog_id_, self.handle_.desc)
                self.stream_ = self.handle_.data

    def has_more(self):
        """ Checks whether the stream has any more elements.

        Returns: True if there are any elements left in the stream, false otherwise.
        """
        return self.handle_.has_more or not self.stream_

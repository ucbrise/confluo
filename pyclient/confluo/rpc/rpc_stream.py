class record_stream:
    
    def __init__(self, multilog_id, schema, client, handle):
        self.multilog_id_ = multilog_id
        self.schema_ = schema
        self.client_ = client
        self.handle_ = handle
        self.cur_off_ = 0

    def __iter__(self):
        while self.has_more():
            yield self.schema_.apply(0, self.handle_.data[self.cur_off_:])
            self.cur_off_ += self.schema_.record_size_
            if self.cur_off_ == len(self.handle_.data) and self.handle_.has_more:
                self.handle_ = client_.get_more(self.multilog_id_, self.handle_.desc)
                self.cur_off_ = 0

    def has_more(self):
        return self.handle_.has_more or self.cur_off_ != len(self.handle_.data)

class alert_stream:

    def __init__(self, multilog_id, client, handle):
        self.multilog_id_ = multilog_id
        self.client_ = client
        self.handle_ = handle
        self.stream_ = handle.data
        
    def __iter__(self):
        while self.has_more():
            end_idx = alert.find("\n")
            alert = self.stream_[:end_idx]
            self.stream_ = self.stream_[end_idx:]
            yield alert
            if not alert and self.handle_.has_more: 
                self.handle_ = self.client_.get_more(self.multilog_id_, self.handle_.desc)
                self.stream_ = self.handle_.data

    def has_more(self):
        return self.handle_.has_more or not self.stream_

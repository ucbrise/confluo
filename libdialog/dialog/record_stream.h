#ifndef DIALOG_DATA_ITERATOR_H_
#define DIALOG_DATA_ITERATOR_H_

#include <unordered_set>

#include "record.h"

namespace dialog {

template<class offset_container, class schema_t, class data_log_t>
class record_stream {
 public:
  typedef std::forward_iterator_tag iterator_category;
  typedef record_t value_type;

  typedef typename offset_container::iterator offset_iterator;

  record_stream(uint64_t version, const offset_container& offsets,
                const schema_t& schema, const data_log_t& data_log)
      : version_(version),
        it_(offsets.begin()),
        end_(offsets.end()),
        schema_(schema),
        data_log_(data_log) {
    if (it_ == end_)
      return;

    skip_invalid();
  }

  record_t get() const {
    uint64_t offset = *it_;
    return schema_.apply(offset, data_log_.cptr(offset), schema_.record_size());
  }

  record_stream& operator++() {
    if (has_more()) {
      ++it_;
      skip_invalid();
    }
    return *this;
  }

  record_stream& operator++(int) {
    record_stream it(*this);
    ++*this;
    return it;
  }

  friend bool operator==(const record_stream& a, const record_stream& b) {
    return a.it_ == b.it_ && a.version_ == b.version_;
  }

  friend bool operator!=(const record_stream& a, const record_stream& b) {
    return !(a == b);
  }

  bool has_more() const {
    return it_ != end_;
  }

 private:
  void skip_invalid() {
    while (it_ != end_ && *it_ >= version_)
      ++it_;
  }

  uint64_t version_;
  offset_iterator it_;
  offset_iterator end_;
  const schema_t& schema_;
  const data_log_t& data_log_;
};

template<class rstream_t>
class filtered_record_stream {
 public:
  filtered_record_stream(const rstream_t& it, const compiled_expression& exp)
      : stream_(it),
        exp_(exp) {
    skip_invalid();
  }

  record_t get() const {
    return stream_.get();
  }

  filtered_record_stream& operator++() {
    if (has_more()) {
      ++stream_;
      skip_invalid();
    }
    return *this;
  }

  filtered_record_stream& operator++(int) {
    filtered_record_stream it(*this);
    ++*this;
    return it;
  }

  bool has_more() const {
    return stream_.has_more();
  }

 private:
  void skip_invalid() {
    while (has_more() && !exp_.test(get()))
      ++stream_;
  }

  rstream_t stream_;
  compiled_expression exp_;
};

template<typename rstream_t>
class union_record_stream {
 public:
  typedef std::vector<rstream_t> stream_vector_t;
  union_record_stream(const stream_vector_t& rstreams)
      : cur_sid_(0),
        rstreams_(rstreams) {
    while (cur_sid_ < rstreams_.size() && !rstreams_[cur_sid_].has_more())
      ++cur_sid_;
    if (has_more())
      seen_.insert(get().log_offset());
  }

  record_t get() const {
    return rstreams_[cur_sid_].get();
  }

  union_record_stream& operator++() {
    advance();
    return *this;
  }

  union_record_stream& operator++(int) {
    union_record_stream it(*this);
    ++*this;
    return it;
  }

  bool has_more() const {
    return cur_sid_ < rstreams_.size();
  }

 private:
  void advance() {
    while (has_more()) {
      bool already_exists = true;
      while (rstreams_[cur_sid_].has_more()
          && (already_exists = (seen_.find(get().log_offset()) != seen_.end()))) {
        ++rstreams_[cur_sid_];
      }

      if (!already_exists) {
        seen_.insert(get().log_offset());
        return;
      }

      ++cur_sid_;
    }
  }

  size_t cur_sid_;
  std::vector<rstream_t> rstreams_;
  std::unordered_set<uint64_t> seen_;
};

}

#endif /* DIALOG_DATA_ITERATOR_H_ */

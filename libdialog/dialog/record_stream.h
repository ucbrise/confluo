#ifndef DIALOG_DATA_ITERATOR_H_
#define DIALOG_DATA_ITERATOR_H_

#include "record.h"

namespace dialog {

template<class offset_iterator, class schema_t, class data_log_t>
class record_stream {
 public:
  typedef std::forward_iterator_tag iterator_category;
  typedef record_t value_type;

  record_stream(uint64_t version, const offset_iterator& it,
                const offset_iterator& end, const schema_t& schema,
                const data_log_t& data_log)
      : version_(version),
        it_(it),
        end_(end),
        schema_(schema),
        data_log_(data_log) {
    if (it_ == end_)
      return;

    advance();
  }

  record_t get() const {
    uint64_t offset = *it_;
    return schema_.apply(offset, data_log_.ptr(offset), schema_.record_size());
  }

  record_stream& operator++() {
    if (it_ != end_)
      advance();
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

 private:
  void advance() {
    while (it_ != end_ && *it_ < version_)
      ++it_;
  }

  uint64_t version_;
  offset_iterator it_;
  const offset_iterator& end_;
  const schema_t& schema_;
  const data_log_t& data_log_;
};

}

#endif /* DIALOG_DATA_ITERATOR_H_ */

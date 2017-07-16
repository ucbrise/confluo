#ifndef DIALOG_DATA_ITERATOR_H_
#define DIALOG_DATA_ITERATOR_H_

#include "record.h"

namespace dialog {

template<class offset_iterator, class schema_t, class data_log_t>
class data_iterator {
 public:
  typedef std::forward_iterator_tag iterator_category;
  typedef void* value_type;
  typedef typename ptrdiff_t difference_type;
  typedef typename value_type& reference;

  data_iterator(uint64_t version, const offset_iterator& it,
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

  reference operator*() const {
    return data_log_.ptr(*it_);
  }

  data_iterator& operator++() {
    if (it_ != end_)
      advance();
    return *this;
  }

  data_iterator& operator++(int) {
    data_iterator it(*this);
    ++*this;
    return it;
  }

  friend bool operator==(const data_iterator& a, const data_iterator& b) {
    return a.it_ == b.it_ && a.version_ == b.version_;
  }

  friend bool operator!=(const data_iterator& a, const data_iterator& b) {
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

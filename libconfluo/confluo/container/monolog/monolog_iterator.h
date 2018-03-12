#ifndef CONFLUO_CONTAINER_MONOLOG_MONOLOG_ITERATOR_H_
#define CONFLUO_CONTAINER_MONOLOG_MONOLOG_ITERATOR_H_

/**
 * Iterator for monologs.
 */
template<typename monolog_impl>
class monolog_iterator : public std::iterator<std::input_iterator_tag,
    typename monolog_impl::value_type, typename monolog_impl::difference_type,
    typename monolog_impl::pointer, typename monolog_impl::reference> {
 public:
  typedef typename monolog_impl::value_type value_type;
  typedef typename monolog_impl::difference_type difference_type;
  typedef typename monolog_impl::pointer pointer;
  typedef typename monolog_impl::reference reference;

  monolog_iterator()
      : impl_(nullptr),
        pos_(0) {
  }

  monolog_iterator(const monolog_impl* impl, size_t pos)
      : impl_(impl),
        pos_(pos) {
  }

  reference operator*() const {
    return impl_->get(pos_);
  }

  // TODO: amortize ptr() cost by only calling it only when necessary.
  pointer operator->() const {
    return impl_->ptr(pos_);
  }

  monolog_iterator& operator++() {
    pos_++;
    return *this;
  }

  monolog_iterator operator++(int) {
    monolog_iterator it = *this;
    ++(*this);
    return it;
  }

  bool operator==(monolog_iterator other) const {
    return (impl_ == other.impl_) && (pos_ == other.pos_);
  }

  bool operator!=(monolog_iterator other) const {
    return !(*this == other);
  }

  monolog_iterator& operator=(const monolog_iterator& other) {
    impl_ = other.impl_;
    pos_ = other.pos_;
    return *this;
  }

 protected:
  void increment_position(size_t size) {
    pos_ += size;
  }

 private:
  const monolog_impl* impl_;
  size_t pos_;
};

/**
 * Iterator for monologs with bucket granularity.
 * TODO: make this work for monolog_exp2 type.
 */
template<typename monolog_impl, size_t BUCKET_SIZE>
class monolog_bucket_iterator : monolog_iterator<monolog_impl> {
 public:
  typedef typename monolog_impl::value_type value_type;
  typedef typename monolog_impl::difference_type difference_type;
  typedef typename monolog_impl::pointer pointer;
  typedef typename monolog_impl::reference reference;

  monolog_bucket_iterator(const monolog_impl* impl, size_t pos)
      : monolog_iterator<monolog_impl>(impl, pos - pos % BUCKET_SIZE) {
  }

  monolog_bucket_iterator& operator++() {
    this->increment_position(BUCKET_SIZE);
    return *this;
  }

};


#endif /* CONFLUO_CONTAINER_MONOLOG_MONOLOG_ITERATOR_H_ */

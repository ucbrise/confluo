#ifndef DIALOG_ITERATORS_H_
#define DIALOG_ITERATORS_H_

#include <iterator>

// Based on answer from:
// https://stackoverflow.com/questions/3623082/flattening-iterator

template<typename outer_iterator>
class flattened_iterator {
 public:
  typedef typename outer_iterator::value_type::const_iterator inner_iterator;

  typedef std::forward_iterator_tag iterator_category;
  typedef typename inner_iterator::value_type value_type;
  typedef typename inner_iterator::difference_type difference_type;
  typedef typename inner_iterator::pointer pointer;
  typedef typename inner_iterator::reference reference;

  flattened_iterator(const outer_iterator& it)
      : outer_(it),
        outer_end_(it) {
  }

  flattened_iterator(const outer_iterator& it, const outer_iterator& end)
      : outer_(it),
        outer_end_(end) {
    if (outer_ == outer_end_)
      return;

    inner_ = outer_->begin();
    skip_invalid();
  }

  reference operator*() const {
    return *inner_;
  }

  pointer operator->() const {
    return &*inner_;
  }

  flattened_iterator& operator++() {
    ++inner_;
    if (inner_ == outer_->end())
      skip_invalid();
    return *this;
  }

  flattened_iterator operator++(int) {
    flattened_iterator it(*this);
    ++*this;
    return it;
  }

  friend bool operator==(const flattened_iterator& a,
                         const flattened_iterator& b) {
    if (a.outer_ != b.outer_)
      return false;

    if (a.outer_ != a.outer_end_ && b.outer_ != b.outer_end_
        && a.inner_ != b.inner_)
      return false;

    return true;
  }

  friend bool operator!=(const flattened_iterator& a,
                         const flattened_iterator& b) {
    return !(a == b);
  }

 private:
  void skip_invalid() {
    while (outer_ != outer_end_ && inner_ == outer_->end()) {
      ++outer_;
      if (outer_ != outer_end_)
        inner_ = outer_->begin();
    }
  }

  outer_iterator outer_;
  outer_iterator outer_end_;
  inner_iterator inner_;
};

template<typename container_t>
class flattened_container {
 public:
  typedef typename container_t::const_iterator container_iterator;
  typedef flattened_iterator<container_iterator> iterator;

  flattened_container(const container_t& container)
      : begin_(container.begin()),
        end_(container.end()) {
  }

  iterator begin() const {
    return iterator(begin_, end_);
  }

  iterator end() const {
    return iterator(end_);
  }

  size_t count() const {
    return std::accumulate(
        begin(), end(), static_cast<size_t>(0),
        [](size_t count, typename iterator::value_type val) {
          return count + 1;
        });
  }

 private:
  container_iterator begin_;
  container_iterator end_;
};

#endif /* DIALOG_ITERATORS_H_ */

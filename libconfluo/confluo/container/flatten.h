#ifndef CONFLUO_CONTAINER_FLATTEN_H_
#define CONFLUO_CONTAINER_FLATTEN_H_

#include <iterator>
#include <numeric>

// Based on answer from:
// https://stackoverflow.com/questions/3623082/flattening-iterator

/**
 * A flattened iterator
 *
 */
template<typename outer_iterator>
class flattened_iterator {
 public:
  /** The inner iterator */
  typedef typename outer_iterator::value_type::const_iterator inner_iterator;

  /** The iterator category */
  typedef std::forward_iterator_tag iterator_category;
  /** The value type */
  typedef typename inner_iterator::value_type value_type;
  /** The difference type */
  typedef typename inner_iterator::difference_type difference_type;
  /** The iterator */
  typedef typename inner_iterator::pointer pointer;
  /** Reference to the inner iterator */
  typedef typename inner_iterator::reference reference;

  flattened_iterator() = default;

  /**
   * Initializes a flattened iterator
   *
   * @param it The outer iterator to initialize this flattened iterator
   */
  flattened_iterator(const outer_iterator& it)
      : outer_(it),
        outer_end_(it) {
  }

  /**
   * Constructs a flattened iterator from the beginning and end of the
   * iterator
   *
   * @param it The outer iterator
   * @param end The end of the outer iterator
   */
  flattened_iterator(const outer_iterator& it, const outer_iterator& end)
      : outer_(it),
        outer_end_(end) {
    if (outer_ == outer_end_)
      return;

    inner_ = outer_->begin();
    skip_invalid();
  }

  /**
   * Dereferences the flattened iterator
   *
   * @return The inner iterator
   */
  reference operator*() const {
    return *inner_;
  }

  /**
   * Gets the value at the inner iterator
   *
   * @return A pointer to the value at the inner iterator
   */
  pointer operator->() const {
    return &*inner_;
  }

  /**
   * Advanced the flattened iterator
   *
   * @return The new flattened iterator at the advanced position
   */
  flattened_iterator& operator++() {
    ++inner_;
    if (inner_ == outer_->end())
      skip_invalid();
    return *this;
  }

  /**
   * Advances the flattened iterator by a specified amount
   *
   * 
   *
   * @return The advanced flattened iterator
   */
  flattened_iterator operator++(int) {
    flattened_iterator it(*this);
    ++*this;
    return it;
  }

  /**
   * Performs an equality comparison between two flattened iterators
   *
   * @param a The first flattened iterator
   * @param b The other flattened iterator
   *
   * @return True if the first flattened iterator is equal to the other
   * flattened iterator, false otherwise
   */
  friend bool operator==(const flattened_iterator& a,
                         const flattened_iterator& b) {
    if (a.outer_ != b.outer_)
      return false;

    if (a.outer_ != a.outer_end_ && b.outer_ != b.outer_end_
        && a.inner_ != b.inner_)
      return false;

    return true;
  }

  /**
   * Performs a not equal comparison between two flattened iterators
   *
   * @param a The first flattened iterator
   * @param b The second flattened iterator
   *
   * @return True if the first flattened iterator is not equal to the 
   * second, false otherwise
   */
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

/**
 * A flattened container
 */
template<typename container_t>
class flattened_container {
 public:
  /** The value type of the container */
  typedef typename container_t::value_type::value_type value_type;
  /** The container iterator */
  typedef typename container_t::const_iterator container_iterator;
  /** The iterator */
  typedef flattened_iterator<container_iterator> iterator;
  /** The constant iterator */
  typedef flattened_iterator<container_iterator> const_iterator;

  /**
   * Constructs a flattened container from another flattend container
   *
   * @param container The container used to intialize this flattend
   * container
   */
  flattened_container(const container_t& container)
      : begin_(container.begin()),
        end_(container.end()) {
  }

  /**
   * Gets the beginning of the iterator
   *
   * @return The iterator at the beginning
   */
  iterator begin() const {
    return iterator(begin_, end_);
  }

  /**
   * Gets the end of the ierator
   *
   * @return The iterator at the end
   */
  iterator end() const {
    return iterator(end_);
  }

  /**
   * Gets the number of elements in the flattened container
   *
   * @return The number of elements in the flattened container
   */
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

#endif /* CONFLUO_CONTAINER_FLATTEN_H_ */

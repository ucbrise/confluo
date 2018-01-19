#ifndef CONFLUO_CONTAINER_LAZY_STREAM_H_
#define CONFLUO_CONTAINER_LAZY_STREAM_H_

#include <memory>
#include <functional>
#include <unordered_set>

namespace confluo {
namespace lazy {

/**
 * Stream of data
 */ 
template<typename T>
class stream {
 public:
  /** The type of the head element */
  using head_t = T;
  /** The pointer to the head of the stream */
  using head_ptr_t = std::shared_ptr<head_t>;

  /** The type of the tail element */
  using tail_t = stream<T>;
  /** The pointer to the tail of the stream */
  using tail_ptr_t = std::shared_ptr<tail_t>;

  /** The type of generator */
  using generator_t = std::function<tail_t()>;
  /** Pointer to the generator */
  using generator_ptr_t = std::shared_ptr<generator_t>;

  static stream<T> nil;

  /**
   * Constructs an empty stream
   */
  stream()
      : head_ptr_(nullptr),
        tail_ptr_(nullptr),
        gen_ptr_(nullptr),
        empty_(true) {
  }

  /**
   * Constructs a stream from another stream
   *
   * @param other The other stream to construct this stream from
   */
  stream(const stream& other)
      : head_ptr_(other.head_ptr_),
        tail_ptr_(other.tail_ptr_),
        gen_ptr_(other.gen_ptr_),
        empty_(other.empty_) {
  }

  /**
   * Constructs a stream from a r value reference 
   *
   * @param other The other stream which is an r value reference
   */
  stream(stream&& other)
      : head_ptr_(std::move(other.head_ptr_)),
        tail_ptr_(std::move(other.tail_ptr_)),
        gen_ptr_(std::move(other.gen_ptr_)),
        empty_(other.empty_) {
  }

  template<typename U>
  using base_type = typename std::remove_const<typename std::remove_reference<U>::type>;

  /* stream<T>(head_t&& head, tail_t&& tail),
  // stream<T>(head_t&& head, const tail_t& tail),
  // stream<T>(const head_t& head, tail_t&& tail),
  // stream<T>(const head_t& head, const tail_t& tail)
  */
  /**
   * Constructs a stream from two r value references
   *
   * @tparam H Head type
   * @tparam U Tail type
   * @param head The head of the stream
   * @param tail The tail of the stream
   */
  template<typename H, typename U, typename std::enable_if<
      std::is_same<tail_t, typename base_type<U>::type>::value, int>::type = 0>
  stream(H&& head, U&& tail)
      : head_ptr_(std::make_shared<head_t>(std::forward<H>(head))),
        tail_ptr_(std::make_shared<tail_t>(std::forward<U>(tail))),
        gen_ptr_(nullptr),
        empty_(false) {
  }

  /* stream<T>(head_t&& head, generator_t&& tail_gen),
  // stream<T>(head_t&& head, const generator_t& tail_gen),
  // stream<T>(const head_t& head, generator_t&& tail_gen),
  // stream<T>(const head_t& head, const generator_t& tail_gen)
  */
  /**
   * Constructs a stream from r value references to the head and generator
   *
   * @tparam H The head type
   * @tparam U generator type
   * @param head The head of the stream
   * @param gen The generator
   */
  template<typename H, typename U,
      typename std::enable_if<
          std::is_convertible<typename base_type<U>::type, generator_t>::value,
          int>::type = 0>
  stream(H&& head, U&& gen)
      : head_ptr_(std::make_shared<head_t>(std::forward<H>(head))),
        tail_ptr_(nullptr),
        gen_ptr_(std::make_shared<generator_t>(std::forward<U>(gen))),
        empty_(false) {
  }

  /**
   * Returns whether the stream is empty
   *
   * @return True if the stream is empty, false otherwise
   */
  bool empty() const {
    return empty_;
  }

  /**
   * Returns the head of the stream
   * @throw range_error Operation invalid on an empty stream
   *
   * @return The head of the stream
   */
  const head_t& head() const {
    if (empty_) {
      throw std::range_error("head() called on empty stream");
    }
    return *head_ptr_;
  }

  /**
   * Returns the tail of the stram
   * @throw range_error Operation invalid on an empty stream
   *
   * @return The tail of the stream
   */
  stream tail() const {
    if (empty_) {
      throw std::range_error("tail() called on empty stream");
    }
    if (tail_ptr_)
      return *tail_ptr_;

    return (*gen_ptr_)();
  }

  // TODO: Add tests
  size_t size() const {
    size_t count = 0;
    stream t = *this;
    while (!t.empty()) {
      ++count;
      t = t.tail();
    }
    return count;
  }

  /**
   * Applies function to every element of the stream
   *
   * @tparam F The type of the function
   * @param f The function r value reference
   */
  template<typename F>
  void for_each(F&& f) const {
    stream t = *this;
    while (!t.empty()) {
      f(t.head());
      t = t.tail();
    }
  }

  /**
   * Attaches head to the result of one less element in the tail
   *
   * @param n The number of elements in the stream
   *
   * @return The resultant stream
   */
  stream take(size_t n) const {
    if (empty_ || n < 1) {
      return nil;
    }

    const tail_t& t = tail();

    return stream(*head_ptr_, [t, n]() -> stream {
      return t.take(n - 1);
    });
  }

  /**
   * Maps a function to every element in the stream
   *
   * @tparam F The type of function
   * @param f The function
   *
   * @return Stream after function is applied
   */
  template<typename F>
  auto map(F&& f) const -> stream<decltype(f(this->head()))> {
    using U = decltype(f(this->head()));
    if (empty_) {
      return stream<U>::nil;
    }

    const tail_t& t = tail();

    return stream<U>(f(*head_ptr_), [t, f]() -> stream<U> {
      return t.map(f);
    });
  }

  // TODO: Add tests
  template<typename U, typename F>
  U fold_left(const U& start, F&& f) const {
    U accum = start;
    stream t = *this;
    while (!t.empty()) {
      accum = f(accum, t.head());
      t = t.tail();
    }
    return accum;
  }

  /**
   * Concatenates two streams together
   *
   * @param other The other stream to concatenate with this stream
   *
   * @return The combined stream
   */
  stream concat(stream other) const {
    if (empty_) {
      return other;
    }

    const tail_t& t = tail();

    return stream(*head_ptr_, [t, other]() -> stream {
      return t.concat(other);
    });
  }

  /**
   * Filters a stream to contain only the elements that satisfy the function
   *
   * @tparam F The type of function
   * @param f The function itself
   *
   * @return The resultant stream
   */
  template<typename F>
  stream filter(F&& f) const {
    if (empty_) {
      return nil;
    }

    const tail_t& t = tail();

    if (f(*head_ptr_)) {
      return stream(*head_ptr_, [t, f]() -> stream {
        return t.filter(f);
      });
    }
    return t.filter(std::forward<F>(f));
  }

  /**
   * Maps function to every element in the stream
   *
   * @tparam F The function type
   * @param f The function
   *
   * @return The resultant stream
   */
  template<typename F>
  auto flat_map(F&& f) const -> decltype(f(this->head())) {
    return flatten(map(std::forward<F>(f)));
  }

  /**
   * Gets all of the distinct elements in the stream
   *
   * @return Stream containing just the distinct elements
   */
  stream distinct() const {
    return distinct(std::make_shared<std::unordered_set<T>>());
  }

  /**
   * Converts a stream to a vector
   *
   * @return A vector containing all of the elements in the stream
   */
  std::vector<T> to_vector() const {
    std::vector<T> v;
    stream t = *this;
    while (!t.empty()) {
      v.push_back(t.head());
      t = t.tail();
    }
    return v;
  }

  /**
   * Assigns another stream to this stream
   *
   * @param other The other stream to assign to this stream
   *
   * @return A pointer to this stream
   */
  stream& operator=(const stream& other) {
    head_ptr_ = other.head_ptr_;
    tail_ptr_ = other.tail_ptr_;
    gen_ptr_ = other.gen_ptr_;
    empty_ = other.empty_;
    return *this;
  }

  /**
   * Assigns an r value reference to a stream to this stream
   *
   * @param other The other stream to assign to this stream
   *
   * @return A pointer to this stream
   */
  stream& operator=(stream&& other) {
    head_ptr_ = std::move(other.head_ptr_);
    tail_ptr_ = std::move(other.tail_ptr_);
    gen_ptr_ = std::move(other.gen_ptr_);
    empty_ = other.empty_;
    return *this;
  }

 private:
  stream distinct(std::shared_ptr<std::unordered_set<T>> seen) const {
    if (empty_) {
      return nil;
    }
    auto v = *head_ptr_;
    const tail_t& t = tail();
    if (seen->find(v) == seen->end()) {
      seen->insert(v);
      return stream(v, [t, seen]() {
        return t.distinct(seen);
      });
    } else {
      return t.distinct(seen);
    }
  }

  head_ptr_t head_ptr_;
  tail_ptr_t tail_ptr_;
  generator_ptr_t gen_ptr_;
  bool empty_;
};

template<typename T>
stream<T> stream<T>::nil;

/**
 * Flattens a stream
 *
 * @tparam U The type of stream elements
 * @param s The stream itself
 *
 * @return The flattened stream
 */
template<typename U>
static stream<U> flatten(stream<stream<U>> s) {
  while (!s.empty() && s.head().empty()) {
    s = s.tail();
  }

  if (s.empty()) {
    return stream<U>::nil;
  }

  stream<U> h = s.head();
  return stream<U>(h.head(), [h, s]() -> stream<U> {
    return h.tail().concat(flatten(s.tail()));
  });
}

/**
 * Converts an iterator to a stream
 *
 * @tparam I The type of iterator
 * @param it The iterator itself
 * @param e The element
 *
 * @return Stream with the elements in the iterator
 */
template<typename I>
static stream<typename I::value_type> iterator_to_stream(I it, I e) {
  if (it == e) {
    return stream<typename I::value_type>::nil;
  }
  return stream<typename I::value_type>(*it, [it, e]() mutable {
    return iterator_to_stream(++it, e);
  });
}

/**
 * Converts a container to a stream
 *
 * @tparam C The type of container
 * @param c The element
 *
 * @return Stream with all of the elements in the iterator
 */
template<typename C>
static stream<typename C::value_type> container_to_stream(C const& c) {
  return iterator_to_stream(c.begin(), c.end());
}

}
}

#endif /* CONFLUO_CONTAINER_LAZY_STREAM_H_ */

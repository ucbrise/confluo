#ifndef CONFLUO_CONTAINER_LAZY_STREAM_H_
#define CONFLUO_CONTAINER_LAZY_STREAM_H_

#include <memory>
#include <functional>
#include <unordered_set>

namespace confluo {
namespace lazy {

template<typename T>
class stream {
 public:
  using head_t = T;
  using head_ptr_t = std::shared_ptr<head_t>;

  using tail_t = stream<T>;
  using tail_ptr_t = std::shared_ptr<tail_t>;

  using generator_t = std::function<tail_t()>;
  using generator_ptr_t = std::shared_ptr<generator_t>;

  static stream<T> nil;

  stream()
      : head_ptr_(nullptr),
        tail_ptr_(nullptr),
        gen_ptr_(nullptr),
        empty_(true) {
  }

  stream(const stream& other)
      : head_ptr_(other.head_ptr_),
        tail_ptr_(other.tail_ptr_),
        gen_ptr_(other.gen_ptr_),
        empty_(other.empty_) {
  }

  stream(stream&& other)
      : head_ptr_(std::move(other.head_ptr_)),
        tail_ptr_(std::move(other.tail_ptr_)),
        gen_ptr_(std::move(other.gen_ptr_)),
        empty_(other.empty_) {
  }

  template<typename U>
  using base_type = typename std::remove_const<typename std::remove_reference<U>::type>;

  // stream<T>(head_t&& head, tail_t&& tail),
  // stream<T>(head_t&& head, const tail_t& tail),
  // stream<T>(const head_t& head, tail_t&& tail),
  // stream<T>(const head_t& head, const tail_t& tail)
  template<typename H, typename U, typename std::enable_if<
      std::is_same<tail_t, typename base_type<U>::type>::value, int>::type = 0>
  stream(H&& head, U&& tail)
      : head_ptr_(std::make_shared<head_t>(std::forward<H>(head))),
        tail_ptr_(std::make_shared<tail_t>(std::forward<U>(tail))),
        gen_ptr_(nullptr),
        empty_(false) {
  }

  // stream<T>(head_t&& head, generator_t&& tail_gen),
  // stream<T>(head_t&& head, const generator_t& tail_gen),
  // stream<T>(const head_t& head, generator_t&& tail_gen),
  // stream<T>(const head_t& head, const generator_t& tail_gen)
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

  bool empty() const {
    return empty_;
  }

  const head_t& head() const {
    if (empty_) {
      throw std::range_error("head() called on empty stream");
    }
    return *head_ptr_;
  }

  stream tail() const {
    if (empty_) {
      throw std::range_error("tail() called on empty stream");
    }
    if (tail_ptr_)
      return *tail_ptr_;

    return (*gen_ptr_)();
  }

  template<typename F>
  void for_each(F&& f) const {
    stream t = *this;
    while (!t.empty()) {
      f(t.head());
      t = t.tail();
    }
  }

  stream take(size_t n) const {
    if (empty_ || n < 1) {
      return nil;
    }

    const tail_t& t = tail();

    return stream(*head_ptr_, [t, n]() -> stream {
      return t.take(n - 1);
    });
  }

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

  stream concat(stream other) const {
    if (empty_) {
      return other;
    }

    const tail_t& t = tail();

    return stream(*head_ptr_, [t, other]() -> stream {
      return t.concat(other);
    });
  }

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

  template<typename F>
  auto flat_map(F&& f) const -> decltype(f(this->head())) {
    return flatten(map(std::forward<F>(f)));
  }

  stream distinct() const {
    return distinct(std::make_shared<std::unordered_set<T>>());
  }

  std::vector<T> to_vector() const {
    std::vector<T> v;
    stream t = *this;
    while (!t.empty()) {
      v.push_back(t.head());
      t = t.tail();
    }
    return v;
  }

  stream& operator=(const stream& other) {
    head_ptr_ = other.head_ptr_;
    tail_ptr_ = other.tail_ptr_;
    gen_ptr_ = other.gen_ptr_;
    empty_ = other.empty_;
    return *this;
  }

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

template<typename I>
static stream<typename I::value_type> iterator_to_stream(I it, I e) {
  if (it == e) {
    return stream<typename I::value_type>::nil;
  }
  return stream<typename I::value_type>(*it, [it, e]() mutable {
    return iterator_to_stream(++it, e);
  });
}

template<typename C>
static stream<typename C::value_type> container_to_stream(C const& c) {
  return iterator_to_stream(c.begin(), c.end());
}

}
}

#endif /* CONFLUO_CONTAINER_LAZY_STREAM_H_ */

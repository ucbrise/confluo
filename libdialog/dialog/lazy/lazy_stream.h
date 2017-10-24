#ifndef LAZY_LAZY_STREAM_H_
#define LAZY_LAZY_STREAM_H_

#include <memory>
#include <functional>
#include <unordered_set>

#include "suspended_function.h"

namespace dialog {
namespace lazy {

template<typename T>
class lazy_stream;

template<typename T>
class cell {
 public:
  cell() = default;

  cell(T const& value, lazy_stream<T> const& tail)
      : value_(value),
        tail_(tail) {
  }

  explicit cell(T const& value)
      : value_(value) {
  }

  T value() const {
    return value_;
  }

  lazy_stream<T> pop_front() const {
    return tail_;
  }

 private:
  T value_;
  lazy_stream<T> tail_;
};

template<typename U>
static lazy_stream<U> flatten(lazy_stream<lazy_stream<U>> s);

template<typename T>
class lazy_stream {
 public:
  using value_t = T;
  using stream_t = lazy_stream<value_t>;
  using cell_t = cell<value_t>;
  using susp_t = suspended_function<cell_t>;

  lazy_stream() = default;

  lazy_stream(std::function<cell<T>()> f)
      : lazy_cell_(std::make_shared<susp_t>(f)) {
  }

  lazy_stream(lazy_stream const& other)
      : lazy_cell_(other.lazy_cell_) {
  }

  lazy_stream(lazy_stream&& other)
      : lazy_cell_(std::move(other.lazy_cell_)) {
  }

  lazy_stream& operator=(lazy_stream&& other) {
    lazy_cell_ = std::move(other.lazy_cell_);
    return *this;
  }

  lazy_stream& operator=(lazy_stream const& other) {
    lazy_cell_ = other.lazy_cell_;
    return *this;
  }

  bool empty() const {
    return !lazy_cell_;
  }

  bool has_more() const {
    return !empty();
  }

  T get() const {
    return lazy_cell_->get().value();
  }

  stream_t pop_front() const {
    return lazy_cell_->get().pop_front();
  }

  stream_t take(size_t n) const {
    if (n == 0 || empty()) {
      return lazy_stream();
    }
    auto c = lazy_cell_;
    return stream_t([c, n]() {
      auto v = c->get().value();
      auto t = c->get().pop_front();
      return cell_t(v, t.take(n - 1));
    });
  }

  template<typename F>
  void for_each(F&& f) {
    stream_t t = *this;
    while (!t.empty()) {
      f(t.get());
      t = t.pop_front();
    }
  }

  template<typename F>
  auto map(F&& f) const -> lazy_stream<decltype(f(get()))> {
    using U = decltype(f(get()));
    if (empty()) {
      return lazy_stream<U>();
    }
    auto v = get();
    auto t = pop_front();
    return lazy_stream<U>([v, t, f]() {
      return cell<U>(f(v), t.map(f));
    });
  }

  stream_t concat(stream_t const& other) const {
    if (empty()) {
      return other;
    }
    auto v = get();
    auto t = pop_front();
    return stream_t([v, t, other, this]() {
      return cell_t(v, t + other);
    });
  }

  stream_t operator+(stream_t const& other) const {
    return concat(other);
  }

  template<typename F>
  stream_t filter(F&& f) const {
    if (empty()) {
      return stream_t();
    }
    auto v = get();
    auto t = pop_front();
    if (f(v)) {
      return stream_t([v, t, f]() {
        return cell_t(v, t.filter(f));
      });
    } else {
      return t.filter(f);
    }
  }

  template<typename F>
  auto flat_map(F&& f) -> decltype(f(get())) const {
    return flatten(map(std::forward<F>(f)));
  }

  std::vector<T> to_vector() const {
    std::vector<T> v;
    stream_t t = *this;
    while (!t.empty()) {
      v.push_back(t.get());
      t = t.pop_front();
    }
    return v;
  }

  stream_t distinct() const {
    return distinct(std::make_shared<std::unordered_set<T>>());
  }

 protected:
  stream_t distinct(std::shared_ptr<std::unordered_set<T>> seen) const {
    if (empty()) {
      return stream_t();
    }
    auto v = get();
    auto t = pop_front();
    if (seen->find(v) == seen->end()) {
      seen->insert(v);
      return stream_t([v, t, seen]() {
        return cell_t(v, t.distinct(seen));
      });
    } else {
      return t.distinct(seen);
    }
  }

  std::shared_ptr<susp_t> lazy_cell_;
};

template<typename U>
static lazy_stream<U> flatten(lazy_stream<lazy_stream<U>> s) {
  while (!s.empty() && s.get().empty()) {
    s = s.pop_front();
  }

  if (s.empty()) {
    return lazy_stream<U>();
  }
  return lazy_stream<U>([s]() {
    auto h = s.get();
    return cell<U>(h.get(), h.pop_front().concat(flatten(s.pop_front())));
  });
}

template<typename iterator>
static lazy_stream<typename iterator::value_type> from_iterator(iterator it,
                                                                iterator e) {
  if (it == e) {
    return lazy_stream<typename iterator::value_type>();
  }
  return lazy_stream<typename iterator::value_type>([it, e]() mutable {
    return cell<typename iterator::value_type>(*it, from_iterator(++it, e));
  });
}

template<typename container>
static lazy_stream<typename container::value_type> from_container(
    container const& c) {
  return from_iterator(c.begin(), c.end());
}

}
}

#endif /* LAZY_LAZY_STREAM_H_ */

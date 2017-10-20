#ifndef LAZY_SUSPENDED_FUNCTION_H_
#define LAZY_SUSPENDED_FUNCTION_H_

namespace dialog {
namespace lazy {

template<typename T>
class suspended_function {
 public:
  explicit suspended_function(std::function<T()> f)
      : thunk_(&thunk_force),
        memoized_(T()),
        f_(f) {
  }

  T get() {
    return thunk_(this);
  }

 private:
  static T const& thunk_force(suspended_function* s) {
    return s->set_memoized();
  }

  static T const& thunk_get(suspended_function* s) {
    return s->get_memoized();
  }

  T const& get_memoized() {
    return memoized_;
  }

  T const& set_memoized() {
    memoized_ = f_();
    thunk_ = &thunk_get;
    return get_memoized();
  }

  T const& (*thunk_)(suspended_function*);
  mutable T memoized_;
  std::function<T()> f_;
};

}
}

#endif /* LAZY_SUSPENDED_FUNCTION_H_ */

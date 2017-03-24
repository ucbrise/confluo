#ifndef ATOMIC_ATOMIC_H_
#define ATOMIC_ATOMIC_H_

#define CPP11_ATOMICS

#ifdef CPP11_ATOMICS
#include <atomic>
#else
#include <stdlib.h>
#include <stdint.h>
#endif

namespace atomic {

#ifdef CPP11_ATOMICS
template<typename T>
using type = std::atomic<T>;
#else
template<typename T>
using type = T;
#endif

namespace weak {
template<typename T>
static inline bool cas(type<T>* obj, T* expected, T desired) {
#ifdef CPP11_ATOMICS
  return std::atomic_compare_exchange_weak_explicit(obj, expected, desired,
                                                    std::memory_order_release,
                                                    std::memory_order_acquire);
#else
  return __atomic_compare_exchange(obj, expected, &desired, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE);
#endif
}
}

namespace strong {
template<typename T>
static inline bool cas(type<T>* obj, T* expected, T desired) {
#ifdef CPP11_ATOMICS
  return std::atomic_compare_exchange_strong_explicit(obj, expected, desired,
                                                      std::memory_order_release,
                                                      std::memory_order_acquire);
#else
  return __atomic_compare_exchange(obj, expected, &desired, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE);
#endif
}
}

template<typename T>
static inline T faa(type<T>* obj, T arg) {
#ifdef CPP11_ATOMICS
  return std::atomic_fetch_add_explicit(obj, arg, std::memory_order_release);
#else
  return __atomic_fetch_add(obj, arg, __ATOMIC_RELEASE);
#endif
}

template<typename T>
static inline T load(const type<T>* obj) {
#ifdef CPP11_ATOMICS
  return std::atomic_load_explicit(obj, std::memory_order_acquire);
#else
  T ret;
  __atomic_load(obj, &ret, __ATOMIC_ACQUIRE);
  return ret;
#endif
}

template<typename T>
static inline void store(type<T>* obj, T arg) {
#ifdef CPP11_ATOMICS
  std::atomic_store_explicit(obj, arg, std::memory_order_release);
#else
  __atomic_store(obj, &arg, __ATOMIC_RELEASE);
#endif
}

template<typename T>
static inline void init(type<T>* obj, T arg) {
#ifdef CPP11_ATOMICS
  obj->store(arg, std::memory_order_relaxed);
#else
  *obj = arg;
#endif
}

}

#endif /* ATOMIC_ATOMIC_H_ */

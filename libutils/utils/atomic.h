#ifndef UTILS_ATOMIC_H_
#define UTILS_ATOMIC_H_

#define CPP11_ATOMICS

#ifdef CPP11_ATOMICS
#include <atomic>
#else
#include <stdlib.h>
#include <stdint.h>
#endif

// Utility wrappers around all atomic primitives
namespace atomic {

// Atomic type
#ifdef CPP11_ATOMICS
template<typename T>
using type = std::atomic<T>;
#else
template<typename T>
using type = T;
#endif

// Weak atomics
namespace weak {

// Compare and swap
template<typename T>
static inline bool cas(type<T> *obj, T *expected, const T &desired) {
#ifdef CPP11_ATOMICS
  return std::atomic_compare_exchange_weak_explicit(obj, expected, desired,
                                                    std::memory_order_release,
                                                    std::memory_order_acquire);
#else
  return __atomic_compare_exchange(obj, expected, &desired, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE);
#endif
}
}

// Strong atomics
namespace strong {

// Compare and swap
template<typename T>
static inline bool cas(type<T> *obj, T *expected, const T &desired) {
#ifdef CPP11_ATOMICS
  return std::atomic_compare_exchange_strong_explicit(obj, expected, desired,
                                                      std::memory_order_release,
                                                      std::memory_order_acquire);
#else
  return __atomic_compare_exchange(obj, expected, &desired, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE);
#endif
}

}

// Exchange
template<typename T>
static inline T exchange(type<T> *obj, const T &desired) {
#ifdef CPP11_ATOMICS
  return std::atomic_exchange_explicit(obj, desired, std::memory_order_acquire);
#else
  T* ret;
  __atomic_exchange(obj, &desired, ret, __ATOMIC_ACQUIRE);
#endif
}

// Fetch and add
template<typename T>
static inline T faa(type<T> *obj, const T &arg) {
#ifdef CPP11_ATOMICS
  return std::atomic_fetch_add_explicit(obj, arg, std::memory_order_release);
#else
  return __atomic_fetch_add(obj, arg, __ATOMIC_RELEASE);
#endif
}

// Fetch and subtract
template<typename T>
static inline T fas(type<T> *obj, const T &arg) {
#ifdef CPP11_ATOMICS
  return std::atomic_fetch_sub_explicit(obj, arg, std::memory_order_release);
#else
  return __atomic_fetch_sub(obj, arg, __ATOMIC_RELEASE);
#endif
}

// Atomic load
template<typename T>
static inline T load(const type<T> *obj) {
#ifdef CPP11_ATOMICS
  return std::atomic_load_explicit(obj, std::memory_order_acquire);
#else
  T ret;
  __atomic_load(obj, &ret, __ATOMIC_ACQUIRE);
  return ret;
#endif
}

// Atomic store
template<typename T>
static inline void store(type<T> *obj, const T &arg) {
#ifdef CPP11_ATOMICS
  std::atomic_store_explicit(obj, arg, std::memory_order_release);
#else
  __atomic_store(obj, &arg, __ATOMIC_RELEASE);
#endif
}

// Atomic init
template<typename T>
static inline void init(type<T> *obj, const T &arg) {
#ifdef CPP11_ATOMICS
  obj->store(arg, std::memory_order_relaxed);
#else
  *obj = arg;
#endif
}

namespace c11 {

namespace weak {
template<typename T>
static inline bool cas(T *obj, T *expected, const T &desired) {
  return __atomic_compare_exchange(obj, expected, &desired, true,
                                   __ATOMIC_RELEASE, __ATOMIC_ACQUIRE);
}
}

namespace strong {
template<typename T>
static inline bool cas(T *obj, T *expected, const T &desired) {
  return __atomic_compare_exchange(obj, expected, &desired, false,
                                   __ATOMIC_RELEASE, __ATOMIC_ACQUIRE);
}
}

template<typename T>
static inline T exchange(T *obj, const T &desired) {
  T *ret;
  __atomic_exchange(obj, &desired, ret, __ATOMIC_ACQUIRE);
}

template<typename T>
static inline T faa(T *obj, const T &arg) {
  return __atomic_fetch_add(obj, arg, __ATOMIC_RELEASE);
}

template<typename T>
static inline T load(const T *obj) {
  T ret;
  __atomic_load(obj, &ret, __ATOMIC_ACQUIRE);
  return ret;
}

// Atomic store
template<typename T>
static inline void store(T *obj, const T &arg) {
  __atomic_store(obj, &arg, __ATOMIC_RELEASE);
}

// Atomic init
template<typename T>
static inline void init(T *obj, const T &arg) {
  *obj = arg;
}

}
}

#endif /* UTILS_ATOMIC_H_ */

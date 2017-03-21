#ifndef ATOMIC_ATOMIC_H_
#define ATOMIC_ATOMIC_H_

#include <atomic>

namespace atomic {

namespace weak {
template<typename T>
static inline bool cas(std::atomic<T>* obj, T* expected, T desired) {
  return std::atomic_compare_exchange_weak_explicit(obj, expected, desired,
                                                    std::memory_order_release,
                                                    std::memory_order_acquire);
}
}

namespace strong {
template<typename T>
static inline bool cas(std::atomic<T>* obj, T* expected, T desired) {
  return std::atomic_compare_exchange_strong_explicit(obj, expected, desired,
                                                      std::memory_order_release,
                                                      std::memory_order_acquire);
}
}

template<typename T>
static inline T faa(std::atomic<T>* obj, T arg) {
  return std::atomic_fetch_add_explicit(obj, arg, std::memory_order_release);
}

template<typename T>
static inline T load(const std::atomic<T>* obj) {
  return std::atomic_load_explicit(obj, std::memory_order_acquire);
}

template<typename T>
static inline void store(std::atomic<T>* obj, T arg)  {
  std::atomic_store_explicit(obj, arg, std::memory_order_release);
}

template<typename T>
static inline void init(std::atomic<T>* obj, T arg) {
  std::atomic_init(obj, arg);
}

}

#endif /* ATOMIC_ATOMIC_H_ */

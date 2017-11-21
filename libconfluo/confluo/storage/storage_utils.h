#ifndef CONFLUO_STORAGE_STORAGE_UTILS_H_
#define CONFLUO_STORAGE_STORAGE_UTILS_H_

namespace confluo {
namespace storage {

template<class T, class enabler = void>
struct destructor_util {
  static void destroy(void* ptr) {
    reinterpret_cast<T*>(ptr)->~T();
  }
};

/**
 * Specialization to do nothing for fundamental types.
 */
template<class T>
struct destructor_util<T, std::enable_if<std::is_fundamental<T>::value>> {
  static void destroy(void* ptr) { }
};

}
}

#endif /* CONFLUO_STORAGE_STORAGE_UTILS_H_ */

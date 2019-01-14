#ifndef CONFLUO_CONTAINER_SKETCH_SKETCH_UTILS_H_
#define CONFLUO_CONTAINER_SKETCH_SKETCH_UTILS_H_

#include <vector>

#include "types/immutable_value.h"

namespace confluo {
namespace sketch {

#define ELEM_SWAP(a,b) { T t=(a); (a)=(b); (b)=t; }

 /**
  *
  * Median Algorithm from N. Wirth's book, implementation by N. Devillard
  * @tparam T type of data
  * @param data data
  * @return median of data
  */
template<typename T>
static T median(std::vector<T>& data) {
  size_t k = (data.size() & 1) ? (data.size() / 2) : (data.size() / 2) - 1;
  size_t i, j, l, m;
  T x;
  l = 0;
  m = data.size() - 1;
  while (l < m) {
    x = data[k];
    i = l;
    j = m;
    do {
      while (data[i] < x)
        i++;
      while (x < data[j])
        j--;
      if (i <= j) {
        ELEM_SWAP(data[i], data[j]);
        i++;
        j--;
      }
    } while (i<= j);

    if (j < k)
      l = i;
    if (k < i)
      m = j;
  }
  return data[k];
}

/**
 * Utility for sketch-specific hashing
 */
struct hash_util {

  static size_t hash(const uint8_t *data, size_t size) {
    size_t data_hash = 5381;
    for (size_t i = 0; i < size; i++)
      data_hash += (data_hash << 5) + data[i]; /* hash * 33 + c */
    return data_hash;
  }

  static size_t hash(const immutable_value &value) {
    return hash(reinterpret_cast<const uint8_t *>(value.ptr()), value.type().size);
  }

};


}
}

#endif /* CONFLUO_CONTAINER_SKETCH_SKETCH_UTILS_H_ */

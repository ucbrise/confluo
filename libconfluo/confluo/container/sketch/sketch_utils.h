#ifndef CONFLUO_CONTAINER_SKETCH_SKETCH_UTILS_H_
#define CONFLUO_CONTAINER_SKETCH_SKETCH_UTILS_H_

#include <vector>

/**
 * Algorithm from N. Wirth's book, implementation by N. Devillard.
 */
namespace confluo {
namespace sketch {

#define ELEM_SWAP(a,b) { T t=(a); (a)=(b); (b)=t; }

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

    if (j<k)
      l = i;
    if (k < i)
      m = j;
  }
  return data[k];
}

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_SKETCH_UTILS_H_ */

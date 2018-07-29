#ifndef CONFLUO_PRIORITY_QUEUE_H
#define CONFLUO_PRIORITY_QUEUE_H

#include <vector>
#include <queue>
#include "rand_utils.h"

namespace confluo {
namespace sketch {

template<typename T, typename P>
struct pq_element {
  pq_element(T key, P priority)
          : key_(key),
            priority_(priority) {
  }

  static bool gt_compare(pq_element x, pq_element y) {
    return x.priority_ > y.priority_;
  }

  static bool lt_compare(pq_element x, pq_element y) {
    return x.priority_ < y.priority_;
  }

  static bool element_eq(const pq_element& x, const pq_element& y) {
    return x.key_ == y.key_;
  }

  T key_;
  P priority_;
};

//template<typename T, typename M, typename P>
//struct pq_element_descript {
//  pq_element_descript(T key, M metadata, P priority)
//      : key_(key),
//        metadata_(metadata),
//        priority_(priority) {
//  }

//  static bool gt_compare(pq_element x, pq_element y) {
//    return x.priority_ > y.priority_;
//  }

//  static bool lt_compare(pq_element x, pq_element y) {
//    return x.priority_ < y.priority_;
//  }

//  static bool element_eq(const pq_element_descript& x, const pq_element_descript& y) {
//    return x.key_ == y.key_;
//  }

//  T key_;
//  M metadata_;
//  P priority_;
//};

template<typename T, typename P, typename E = pq_element<T, P>>
class heavy_hitter_set : public std::priority_queue<E, std::vector<E>, std::function<bool(E, E)>> {

public:

  heavy_hitter_set()
      : std::priority_queue<E, std::vector<E>, std::function<bool(E, E)>>(E::gt_compare) {
  }

  void pushp(T key, P priority) {
    this->push(E(key, priority));
  }

  void update(T key, P priority) {
   this->remove_if_exists(key);
   this->pushp(key, priority);
  }

  bool remove_if_exists(const T& key) {
    E query(key, P());
    auto it = std::find_if(this->c.begin(), this->c.end(), std::bind(E::element_eq, std::placeholders::_1, query));
    if (it != this->c.end()) {
      this->c.erase(it);
      std::make_heap(this->c.begin(), this->c.end(), this->comp);
      return true;
    }
    else {
      return false;
    }
  }

  typename std::vector<E>::const_iterator begin() {
    return std::priority_queue<E, std::vector<E>, std::function<bool(E, E)>>::c.begin();
  }

  typename std::vector<E>::const_iterator end() {
    return std::priority_queue<E, std::vector<E>, std::function<bool(E, E)>>::c.end();
  }

  size_t storage_size() {
    return this->c.size() * sizeof(E);
  }
};

}
}

#endif //CONFLUO_PRIORITY_QUEUE_H

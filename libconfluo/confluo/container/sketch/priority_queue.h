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
          : key(key),
            priority(priority) {
  }

  static bool gt_compare(pq_element x, pq_element y) {
    return x.priority > y.priority;
  }

  static bool lt_compare(pq_element x, pq_element y) {
    return x.priority < y.priority;
  }

  static bool element_eq(const pq_element& x, const pq_element& y) {
    return x.key == y.key;
  }

  T key;
  P priority;
};

/**
 *
 * This priority queue is not thread-safe.
 * @tparam T type of key
 * @tparam P type of priority
 * @tparam E type of element to store, containing key and priority
 */
template<typename T, typename P, typename E = pq_element<T, P>>
class thread_unsafe_pq : public std::priority_queue<E, std::vector<E>, std::function<bool(E, E)>> {

public:

  /**
   * Constructor
   * @param compare comparison function (default behavior: min-pq)
   */
  thread_unsafe_pq(std::function<bool(E, E)> compare = E::gt_compare)
      : std::priority_queue<E, std::vector<E>, std::function<bool(E, E)>>(compare) {
  }

  /**
   * Inserts a new element into the priority queue. Does not perform de-duplication.
   * @param key key of element to push
   * @param priority key's priority
   */
  void pushp(T key, P priority) {
    this->push(E(key, priority));
  }

  /**
   * Updates the key's priority if it already exists.
   * @param key key of element to update
   * @param priority key's new priority
   * @param insert_if_not_found if the key isn't found insert a new element for it
   */
  bool update(T key, P priority, bool insert_if_not_found = false) {
   bool found = this->remove_if_exists(key);
   if (found || insert_if_not_found) {
     this->pushp(key, priority);
     return true;
   }
   return false;
  }

  /**
   * Checks if priority queue contains element.
   * @param key key of element
   * @return true if priority queue contains element, else false
   */
  bool contains(T key) {
    E query(key, P());
    auto it = std::find_if(this->c.begin(), this->c.end(), std::bind(E::element_eq, std::placeholders::_1, query));
    return it != this->c.end();
  }

  /**
   * Removes from priority queue if key already exists.
   * @param key key of element to remove
   * @return true iff the key was found and the element was removed
   */
  bool remove_if_exists(const T& key) {
    E query(key, P());
    auto it = std::find_if(this->c.begin(), this->c.end(), std::bind(E::element_eq, std::placeholders::_1, query));
    if (it != this->c.end()) {
      this->c.erase(it);
      std::make_heap(this->c.begin(), this->c.end(), this->comp);
      return true;
    }
    return false;
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

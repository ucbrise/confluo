#ifndef DIALOG_AGGREGATES_H_
#define DIALOG_AGGREGATES_H_

#include "atomic.h"

namespace dialog {
namespace aggregate {

template<typename T, typename update_functor>
struct aggregate_node {
  typedef aggregate_node<T, update_functor> aggregate_node_t;
  static_assert(std::is_integral<T>::value, "Aggregate must be integral");

  aggregate_node(T agg, T version, aggregate_node_t *next)
      : agg_(agg),
        version_(version),
        next_(next) {
  }

  void update_value(T value) {
    T expected = atomic::load(&agg_);
    while (!atomic::weak::cas(&agg_, expected,
                              update_functor::apply(agg_, value)))
      ;
  }

  atomic::type<T> agg_;
  uint64_t version_;
  aggregate_node_t* next_;
};

template<typename T, typename update_functor>
class aggregate {
 public:
  typedef aggregate_node<T, update_functor> aggregate_node_t;

  aggregate() = default;
  ~aggregate() = default;

  T get(uint64_t version) {
    aggregate_node_t *node = atomic::load(&head_);
    while (node && node->version_ != version)
      node = node->next_;
    return atomic::load(&node->agg_);
  }

 private:
  aggregate_node_t* closest_version(uint64_t version) {
    return nullptr;
  }

  atomic::type<aggregate_node_t*> head_;
};

}
}

#endif /* DIALOG_AGGREGATES_H_ */

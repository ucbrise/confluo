#ifndef CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_
#define CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_

#include <vector>
#include "container/monolog/monolog_exp2.h"
#include "rand_utils.h"

namespace confluo {
namespace sketch {

// TODO rename
class simple_hash {

 public:
  static const size_t PRIME = 4294967311;

  simple_hash()
      : simple_hash(0, 0) {
  }

  simple_hash(size_t a, size_t b)
      : a_(a),
        b_(b) {
  }

  template<typename T>
  typename std::enable_if<!std::is_arithmetic<T>::value, size_t>::type apply(T elem) {
    return (a_ * std::hash<T>(elem) + b_) % PRIME;
  }

  template<typename T>
  typename std::enable_if<std::is_arithmetic<T>::value, size_t>::type apply(T elem) {
    return (a_ * elem + b_) % PRIME;
  }

  static simple_hash generate_random() {
    return simple_hash(utils::rand_utils::rand_uint64(PRIME), utils::rand_utils::rand_uint64(PRIME));
  }

 private:
  size_t a_, b_;

};

const size_t simple_hash::PRIME;

class hash_manager {
 public:
  /**
   * Constructor.
   * @param num_hashes number of hashes
   */
  hash_manager(size_t num_hashes = 0)
      : hashes_() {
    this->guarantee_initialized(num_hashes);
  }

  /**
   * Guarantee enough hashes are intialized.
   * @param num_hashes number of hashes
   */
  void guarantee_initialized(size_t num_hashes) {
    size_t cur_size = hashes_.size();
    size_t num_new_hashes = num_hashes > cur_size ? num_hashes - cur_size : 0;
    for (size_t i = 0; i < num_new_hashes; i++) {
      hashes_.push_back(simple_hash::generate_random());
    }
  }

  /**
   * Hash element.
   * @param hash_id id of hash to use
   * @param elem element to hash
   * @return hashed value
   */
  template<typename T>
  size_t hash(size_t hash_id, T elem) {
    return hashes_[hash_id].apply<T>(elem);
  }

 private:
  std::vector<simple_hash> hashes_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_ */

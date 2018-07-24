#ifndef CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_
#define CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_

#include <vector>
#include "rand_utils.h"

namespace confluo {
namespace sketch {

/**
 * Pairwise-independent hash
 */
class pairwise_indep_hash {

 public:
  static const size_t PRIME = 39916801UL;

  pairwise_indep_hash()
      : pairwise_indep_hash(0, 0) {
  }

  pairwise_indep_hash(size_t a, size_t b)
      : a_(a),
        b_(b) {
  }

  template<typename T>
  size_t apply(T elem) const {
    static std::hash<T> hash;
    return (a_ * hash(elem) + b_) % PRIME;
  }

  template<typename T>
  typename std::enable_if<!std::is_unsigned<T>::value, bool>::type apply(T elem) {
    return (a_ * elem + b_) % PRIME;
  }

  static pairwise_indep_hash generate_random() {
    return { utils::rand_utils::rand_uint64(PRIME), utils::rand_utils::rand_uint64(PRIME) };
  }

 private:
  size_t a_, b_;

};

const size_t pairwise_indep_hash::PRIME;

template<typename T>
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
      hashes_.push_back(pairwise_indep_hash::generate_random());
    }
  }

  /**
   * Hash element.
   * @param hash_id id of hash to use
   * @param elem element to hash
   * @return hashed value
   */
  size_t hash(size_t hash_id, T elem) const {
    return hashes_[hash_id].template apply<T>(elem);
  }

 private:
  std::vector<pairwise_indep_hash> hashes_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_ */

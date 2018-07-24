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
  static const size_t PRIME;

  pairwise_indep_hash();

  pairwise_indep_hash(size_t a, size_t b);

  template<typename T>
  size_t apply(T elem) const {
    static std::hash<T> hash;
    return (a_ * hash(elem) + b_) % PRIME;
  }

  template<typename T>
  typename std::enable_if<!std::is_unsigned<T>::value, bool>::type apply(T elem) {
    return (a_ * elem + b_) % PRIME;
  }

  static pairwise_indep_hash generate_random();

 private:
  size_t a_, b_;

};

class hash_manager {
 public:

  /**
   * Constructor.
   * @param num_hashes number of hashes
   */
  explicit hash_manager(size_t num_hashes = 0);

  /**
   * Guarantee enough hashes are intialized.
   * @param num_hashes number of hashes
   */
  void guarantee_initialized(size_t num_hashes);

  /**
   * Hash element.
   * @param hash_id id of hash to use
   * @param elem element to hash
   * @return hashed value
   */
  template<typename T>
  size_t hash(size_t hash_id, T elem) const {
    return hashes_[hash_id].template apply<T>(elem);
  }

 private:
  std::vector<pairwise_indep_hash> hashes_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_ */

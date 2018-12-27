#ifndef CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_
#define CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_

#include <vector>
#include <iostream>
#include "rand_utils.h"

namespace confluo {
namespace sketch {

/**
 * Pairwise-independent hash
 */
class pairwise_indep_hash {

 public:
  // A large prime
  static const size_t PRIME;

  pairwise_indep_hash();

  pairwise_indep_hash(size_t a, size_t b);

  /**
   * Apply hash to a key
   * @tparam T type of key
   * @param key key to hash
   * @return hashed key
   */
  template<typename T>
  typename std::enable_if<!std::is_integral<T>::value, size_t>::type apply(T key) const {
    static std::hash<T> hash;
    return (a_ * hash(key) + b_) % PRIME;
  }

  /**
   * Template specialization for numeric types
   * @tparam T type of key
   * @param key key to hash
   * @return hashed key
   */
  template<typename T>
  typename std::enable_if<std::is_integral<T>::value, size_t>::type apply(T key) const {
    return (a_ * key + b_) % PRIME;
  }

  bool operator==(const pairwise_indep_hash &other) const {
    return a_ == other.a_ && b_ == other.b_;
  }

  bool operator!=(const pairwise_indep_hash &other) const {
    return !(*this == other);
  }

  /**
   * @return an instance of pairwise_indep_hash with random parameters
   */
  static pairwise_indep_hash generate_random();

 private:
  size_t a_, b_;

};

class hash_manager {
 public:

  /**
   * Constructor
   * @param num_hashes number of hashes
   */
  explicit hash_manager(size_t num_hashes = 0);

  /**
   * Guarantee enough hashes are initialized
   * @param num_hashes number of hashes
   */
  void guarantee_initialized(size_t num_hashes);

  /**
   * Hashes a key of arbitrary type
   * @param hash_id id of hash to use
   * @param key key to hash
   * @return hashed value
   */
  template<typename T>
  size_t hash(size_t hash_id, T key) const {
    return hashes_[hash_id].template apply<T>(key);
  }

  /**
   * @return the number of hashes
   */
  size_t size() const;

  /**
   * @return storage size in bytes
   */
  size_t storage_size() const;

  bool operator==(const hash_manager &other) const {
    for (size_t i = 0; i < hashes_.size(); i++) {
     if (hashes_[i] != other.hashes_[i])
       return false;
    }
    return true;
  }

  bool operator!=(const hash_manager &other) const {
    return !(*this == other);
  }

 private:
  std::vector<pairwise_indep_hash> hashes_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_ */

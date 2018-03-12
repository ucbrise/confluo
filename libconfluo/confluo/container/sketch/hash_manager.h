#ifndef CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_
#define CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_

#include <vector>
#include "rand_utils.h"

namespace confluo {
namespace sketch {

// TODO resolve all race conditions

// TODO rename
class simple_hash {

 public:
  static const size_t LONG_PRIME = 4294967311;

  simple_hash()
      : simple_hash(0, 0) {
  }

  simple_hash(int a, int b)
      : a_(a),
        b_(b) {
  }

  template<typename T>
  //    size_t operator()(T elem) { TODO: can i template overriden func call?
  size_t apply(T elem) {
    std::string str(reinterpret_cast<const char*>(&elem), sizeof(T));
    return (a_ * std::hash<std::string>{}(str) + b_) % LONG_PRIME;
  }

  static simple_hash generate_random() {
    return simple_hash(utils::rand_utils::rand_uint64(LONG_PRIME), utils::rand_utils::rand_uint64(LONG_PRIME));
  }

 private:
  size_t a_, b_;
};

const size_t simple_hash::LONG_PRIME;

class hash_manager {
 public:
  hash_manager(size_t num_hashes = 1)
      : hashes_(num_hashes) {
  }

  void guarantee_initialized(size_t num_hashes) {
    size_t num_new_hashes = num_hashes > hashes_.size() ? num_hashes - hashes_.size() : 0;
    for (size_t i = 0; i < num_new_hashes; i++)
      hashes_.push_back(simple_hash::generate_random());
  }

  template<typename T>
  size_t hash(T elem, size_t hash_id) {
    return hashes_[hash_id].apply<T>(elem);
  }

 private:
  std::vector<simple_hash> hashes_;

};

}
}

#endif /* CONFLUO_CONTAINER_SKETCH_HASH_MANAGER_H_ */

#include "container/sketch/hash_manager.h"

namespace confluo {
namespace sketch {

const size_t pairwise_indep_hash::PRIME = 39916801UL;

pairwise_indep_hash::pairwise_indep_hash()
    : pairwise_indep_hash(0, 0) {
}

pairwise_indep_hash::pairwise_indep_hash(size_t a, size_t b)
        : a_(a),
          b_(b) {
}

pairwise_indep_hash pairwise_indep_hash::generate_random() {
  return { utils::rand_utils::rand_uint64(PRIME), utils::rand_utils::rand_uint64(PRIME) };
}

hash_manager::hash_manager(size_t num_hashes)
        : hashes_() {
  this->guarantee_initialized(num_hashes);
}

void hash_manager::guarantee_initialized(size_t num_hashes) {
  size_t cur_size = hashes_.size();
  size_t num_new_hashes = num_hashes > cur_size ? num_hashes - cur_size : 0;
  for (size_t i = 0; i < num_new_hashes; i++) {
    hashes_.push_back(pairwise_indep_hash::generate_random());
  }
}

size_t hash_manager::size() const {
  return hashes_.size();
}

size_t hash_manager::storage_size() const {
  return sizeof(pairwise_indep_hash) * hashes_.capacity();
}

}
}

#include "mica_store.h"

#include "hash.h"
#include <cassert>

namespace mica {

MicaStore::MicaStore(const uint32_t value_size, const uint32_t key_size) {

  // Store key/value sizes
  key_size_ = key_size;
  value_size_ = value_size;

  // Initial state
  num_keys_ = 0;
  current_size_ = 0;

  // Allocate memory for table table, value buffer
  table_ = new struct mehcached_table;
  assert(table_);
  value_buf_ = new uint8_t[value_size_];
  assert(value_buf_);

  // Compute num buckets
  size_t num_items = kStoreSize / value_size;
  size_t n_buckets = (num_items + MEHCACHED_ITEMS_PER_BUCKET - 1)
      / MEHCACHED_ITEMS_PER_BUCKET;

  // Compute pool size
  size_t alloc_overhead = sizeof(struct mehcached_item);
#ifdef MEHCACHED_ALLOC_DYNAMIC
  alloc_overhead += MEHCAHCED_DYNAMIC_OVERHEAD;
#endif
  size_t pool_size = num_items * (alloc_overhead + key_size + value_size);

  // Set table numa nodes
  size_t numa_nodes[] = { (size_t) -1 };

  // Initialize table
  mehcached_table_init(table_, n_buckets, 1UL, pool_size, false, false,
  false,
                       numa_nodes[0], numa_nodes,
                       MEHCACHED_MTH_THRESHOLD_FIFO);
}

int MicaStore::Append(const int64_t key, const std::string& value) {
  uint64_t key_hash = hash((const uint8_t *) &key, sizeof(key));
  int ret = mehcached_set(0, table_, key_hash, (const uint8_t *) &key,
                          sizeof(key), (const uint8_t *) value.c_str(),
                          value.size(), 0, false);
  if (ret) {
    num_keys_++;
    current_size_ += value.size();
  }

  return ret;
}

void MicaStore::Get(std::string& value, const int64_t key) {
  uint64_t key_hash = hash((const uint8_t *) &key, sizeof(key));
  size_t value_len;
  int ret = mehcached_get(0, table_, key_hash, (const uint8_t *) &key,
                          sizeof(key), (uint8_t *) value_buf_, &value_len, NULL,
                          false);
  assert(ret);
  value = std::string((const char*) value_buf_);
}

}

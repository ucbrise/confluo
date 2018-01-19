#ifndef CONFLUO_CONTAINER_STRING_MAP_H_
#define CONFLUO_CONTAINER_STRING_MAP_H_

#include <unistd.h>

#include "reflog.h"

namespace confluo {

/**
* String map
*
* @tparam V The type of map
*/
template<typename V>
class string_map {
 public:
  /** The maximum number of buckets the string map can have */
  static const uint32_t MAX_BUCKETS;

  /**
   * The hash computed on a string
   */
  struct string_hash {
    /**
     * Computes the hash for the given string
     * @param str The string to compute the hash on
     * @return The hashed value
     */
    static uint32_t hash(const std::string& str) {
      uint32_t h = FIRSTH;
      const char* s = str.c_str();
      while (*s) {
        h = (h * A) ^ (s[0] * B);
        s++;
      }
      return h % MAX_BUCKETS;  // or return h % C;
    }

   private:
    static const uint32_t A = 54059; /* a prime */
    static const uint32_t B = 76963; /* another prime */
    static const uint32_t C = 86969; /* yet another prime */
    static const uint32_t FIRSTH = 37; /* also prime */
  };

  /**
   * Map entry for the string map
   */
  struct map_entry {
    /** The map key */
    std::string key;
    /** The map value */
    V value;
    /** Whether the entry is valid */
    bool valid;

    /**
     * Constructs a map entry from the given key and value passed in
     *
     * @param k The key of the entry
     * @param v The value of the entry
     */
    map_entry(const std::string& k, const V& v)
        : key(k),
          value(v),
          valid(true) {
    }

    /**
     * Initializes the map entry to be invalid
     */
    map_entry()
        : valid(false) {
    }
  };

  /**
   * Constructs a default string map to have the maximum number of buckets
   */
  string_map()
      : buckets_(new reflog*[MAX_BUCKETS]()) {
  }

  /**
   * Deallocates the string map
   */
  ~string_map() {
    for (size_t i = 0; i < MAX_BUCKETS; i++) {
      if (buckets_[i] == nullptr)
        delete buckets_[i];
    }
    delete[] buckets_;
  }

  // Only one writer thread permitted, otherwise duplicates may occur
  /**
   * Puts a key value pair in the map, only one writer thread permitted,
   * otherwise duplicates may occur
   *
   * @param key The key
   * @param value The value
   *
   * @return The index of the key value pair
   */
  int64_t put(const std::string& key, const V& value) {
    uint32_t hash_key = string_hash::hash(key);
    map_entry entry(key, value);

    reflog* refs;
    if ((refs = buckets_[hash_key]) == nullptr)
      refs = buckets_[hash_key] = new reflog;

    // Search to see if key already exists
    size_t size = refs->size();
    for (size_t i = 0; i < size; i++) {
      int64_t idx = refs->at(i);
      if (entries_.at(idx).key == key) {
        if (entries_.at(idx).valid) {
          return -1;
        } else {
          entries_[idx].value = value;
          entries_[idx].valid = true;
          return idx;
        }
      }
    }

    uint64_t idx = entries_.push_back(entry);
    refs->push_back(idx);
    return static_cast<int64_t>(idx);
  }

  // Multiple threads permitted
  /**
   * Gets the value at a given key, multiple threads permitted
   *
   * @param key The key
   * @param value The value
   *
   * @return The index of the key value pair
   */
  int64_t get(const std::string& key, V& value) const {
    uint32_t hash_key = string_hash::hash(key);
    reflog* refs;
    if ((refs = buckets_[hash_key]) == nullptr) {
      return -1;
    }

    // Search to see if key exists
    size_t size = refs->size();
    for (size_t i = 0; i < size; i++) {
      int64_t idx = refs->at(i);
      const map_entry& entry = entries_.at(idx);
      if (entry.key == key && entry.valid) {
        value = entry.value;
        return idx;
      }
    }
    return -1;
  }

  /**
   * Gets the value at a particular index
   *
   * @param idx The index to get the value at
   * @param value The value at the index
   *
   * @return The index, or -1 if it doesn't exist
   */
  int64_t get(int64_t idx, V& value) const {
    if (idx < entries_.size()) {
      const map_entry& entry = entries_.at(idx);
      if (entry.valid) {
        value = entry.value;
        return idx;
      }
    }
    return -1;
  }

  // Only one writer thread permitted, otherwise inconsistencies may occur
  /**
   * Removes a key value pair from the string map, only one writer thread
   * permitted, otherwise inconsistencies may occur
   *
   * @param key The key
   * @param value The value
   *
   * @return The index of the removed key value pair or -1 if it doesn't
   * exist
   */
  int64_t remove(const std::string& key, V& value) {
    uint32_t hash_key = string_hash::hash(key);
    reflog* refs;
    if ((refs = buckets_[hash_key]) == nullptr)
      return -1;

    // Search to see if key exists
    size_t size = refs->size();
    for (size_t i = 0; i < size; i++) {
      uint64_t idx = refs->at(i);
      map_entry& entry = entries_[idx];
      if (entry.key == key && entry.valid) {
        entry.valid = false;
        value = entry.value;
        return idx;
      }
    }
    return -1;
  }

 private:
  reflog** buckets_;
  monolog::monolog_exp2<map_entry> entries_;
};

template<typename V>
const uint32_t string_map<V>::MAX_BUCKETS = sysconf(_SC_PAGESIZE)
    / sizeof(reflog*);

}

#endif /* CONFLUO_CONTAINER_STRING_MAP_H_ */

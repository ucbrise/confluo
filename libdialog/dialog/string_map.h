#ifndef DIALOG_STRING_MAP_H_
#define DIALOG_STRING_MAP_H_

#include <unistd.h>
#include "reflog.h"

namespace dialog {

template<typename V>
class string_map {
 public:
  static const uint32_t MAX_BUCKETS;

  struct string_hash {
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

  struct map_entry {
    std::string key;
    V value;
    bool valid;

    map_entry(const std::string& k, const V& v)
        : key(k),
          value(v),
          valid(true) {
    }

    map_entry()
        : valid(false) {
    }
  };

  string_map()
      : buckets_(new reflog*[MAX_BUCKETS]()) {
  }

  ~string_map() {
    for (size_t i = 0; i < MAX_BUCKETS; i++) {
      if (buckets_[i] == nullptr)
        delete buckets_[i];
    }
    delete[] buckets_;
  }

  // Only one writer thread permitted, otherwise duplicates may occur
  bool put(const std::string& key, const V& value) {
    uint32_t hash_key = string_hash::hash(key);
    map_entry entry(key, value);

    reflog* refs;
    if ((refs = buckets_[hash_key]) == nullptr)
      refs = buckets_[hash_key] = new reflog;

    // Search to see if key already exists
    size_t size = refs->size();
    for (size_t i = 0; i < size; i++) {
      uint64_t offset = refs->at(i);
      if (entries_.at(offset).key == key) {
        if (entries_.at(offset).valid)
          return false;
        else {
          entries_[offset].value = value;
          entries_[offset].valid = true;
          return true;
        }
      }
    }

    refs->push_back(entries_.push_back(entry));
    return true;
  }

  // Multiple threads permitted
  bool get(const std::string& key, V& value) const {
    uint32_t hash_key = string_hash::hash(key);
    reflog* refs;
    if ((refs = buckets_[hash_key]) == nullptr)
      return false;

    // Search to see if key exists
    size_t size = refs->size();
    for (size_t i = 0; i < size; i++) {
      uint64_t offset = refs->at(i);
      const map_entry& entry = entries_.at(offset);
      if (entry.key == key && entry.valid) {
        value = entries_.at(offset).value;
        return true;
      }
    }
    return false;
  }

  // Only one writer thread permitted, otherwise inconsistencies may occur
  bool remove(const std::string& key, V& value) {
    uint32_t hash_key = string_hash::hash(key);
    reflog* refs;
    if ((refs = buckets_[hash_key]) == nullptr)
      return false;

    // Search to see if key exists
    size_t size = refs->size();
    for (size_t i = 0; i < size; i++) {
      uint64_t offset = refs->at(i);
      map_entry& entry = entries_[offset];
      if (entry.key == key && entry.valid) {
        entry.valid = false;
        value = entry.value;
        return true;
      }
    }
    return false;
  }

 private:
  reflog** buckets_;
  monolog::monolog_exp2<map_entry> entries_;
};

template<typename V>
const uint32_t string_map<V>::MAX_BUCKETS = sysconf(_SC_PAGESIZE)
    / sizeof(reflog*);

}

#endif /* DIALOG_STRING_MAP_H_ */

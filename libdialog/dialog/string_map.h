#ifndef DIALOG_STRING_MAP_H_
#define DIALOG_STRING_MAP_H_

#include "radix_tree.h"

namespace dialog {

template<typename V>
class string_map {
 public:
  static const size_t PREFIX_LEN = 4;

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
      : tree_(PREFIX_LEN, 256) {
  }

  // Only one writer thread permitted, otherwise duplicates may occur
  bool put(const std::string& key, const V& value) {
    byte_string rt_key(key, PREFIX_LEN);
    map_entry entry(key, value);
    reflog* refs = tree_[rt_key];
    if (refs == nullptr) {
      tree_.insert(rt_key, entries_.push_back(entry));
      return true;
    }

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

    tree_.insert(rt_key, entries_.push_back(entry));
    return true;
  }

  // Multiple threads permitted
  bool get(const std::string& key, V& value) const {
    byte_string rt_key(key, PREFIX_LEN);
    reflog const* refs = tree_.at(rt_key);
    if (refs == nullptr)
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
    byte_string rt_key(key, PREFIX_LEN);
    reflog* refs = tree_[rt_key];
    if (refs == nullptr)
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
  index::radix_tree tree_;
  monolog::monolog_exp2<map_entry> entries_;
};

}

#endif /* DIALOG_STRING_MAP_H_ */

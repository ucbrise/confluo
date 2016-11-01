#ifndef SLOG_INDEXLOG_H_
#define SLOG_INDEXLOG_H_

#include <map>
#include <unordered_map>

#include "entrylist.h"
#include "monolog.h"

template<uint32_t TOKEN_LEN, uint32_t PREFIX_LEN>
class token_ops {
 public:
  static uint32_t prefix(const unsigned char* token) {
    uint32_t prefix_val = 0;
    for (uint32_t i = 0; i < PREFIX_LEN; i++) {
      prefix_val = prefix_val * 256 + token[i];
    }
    return prefix_val;
  }

  static uint32_t suffix(const unsigned char* token) {
    uint32_t suffix_val = 0;
    for (uint32_t i = PREFIX_LEN; i < TOKEN_LEN; i++) {
      suffix_val = suffix_val * 256 + token[i];
    }
    return suffix_val;
  }
};

template<>
class token_ops<4, 3> {
 public:
  static uint32_t prefix(const unsigned char* token) {
    return token[0] * 65536 + token[1] * 256 + token[2];
  }

  static uint32_t suffix(const unsigned char* token) {
    return token[3];
  }
};

template<>
class token_ops<3, 3> {
 public:
  static uint32_t prefix(const unsigned char* token) {
    return token[0] * 65536 + token[1] * 256 + token[2];
  }

  static uint32_t suffix(const unsigned char* token) {
    return 0;
  }
};

template<>
class token_ops<2, 2> {
 public:
  static uint32_t prefix(const unsigned char* token) {
    return token[0] * 256 + token[1];
  }

  static uint32_t suffix(const unsigned char* token) {
    return 0;
  }
};

namespace slog {
template<uint32_t TOKEN_LEN = 4, uint32_t PREFIX_LEN = 3>
class indexlog {
  static_assert(TOKEN_LEN >= PREFIX_LEN, "Token length cannot be smaller than prefix length.");
  static_assert(PREFIX_LEN <= 3, "Prefix length too large.");
  static_assert(PREFIX_LEN > 0, "Prefix length cannot be zero.");
  static_assert(TOKEN_LEN - PREFIX_LEN < 4, "Suffix length must be smaller than 4.");

 public:
  typedef std::atomic<entry_list*> atomic_ref;

  indexlog() {
    entry_list* null_ptr = NULL;
    for (auto& x : idx_)
      x = null_ptr;
  }

  uint32_t add_entry(const unsigned char* token, const uint32_t record_id) {
    uint32_t prefix = token_ops<TOKEN_LEN, PREFIX_LEN>::prefix(token);
    uint64_t suffix = token_ops<TOKEN_LEN, PREFIX_LEN>::suffix(token);
    if (idx_[prefix] == NULL) {
      try_allocate_list(prefix);
    }
    entry_list* list = idx_[prefix];
    uint64_t entry = (suffix << 32) | record_id;
    list->push_back(entry);
    return prefix;
  }

  entry_list* get_entry_list(const unsigned char* token) {
    uint32_t prefix = token_ops<TOKEN_LEN, PREFIX_LEN>::prefix(token);
    return idx_[prefix];
  }

  entry_list* get_entry_list(uint32_t idx) {
    return idx_[idx];
  }

  size_t storage_size() {
    size_t array_size = idx_.size() * sizeof(atomic_ref);
    size_t data_size = 0;
    for (uint32_t i = 0; i < idx_.size(); i++) {
      entry_list* entry = idx_[i].load();
      if (entry != NULL) {
        data_size += entry->storage_size();
      }
    }

    return array_size + data_size;
  }

 private:
  void try_allocate_list(uint32_t i) {
    slog::entry_list* new_list = new slog::entry_list;
    slog::entry_list* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_strong(&idx_[i], &null_ptr, new_list))
      return;

    // All other threads will deallocate the newly allocated bucket.
    delete new_list;
  }

  // Not atomic!
  size_t size() {
    size_t sz = 0;
    for (auto& x : idx_)
      if (x.load() != NULL)
        sz += x.load()->size();
    return sz;
  }

  std::array<atomic_ref, 1 << (PREFIX_LEN * 8)> idx_;
};

}

#endif /* SLOG_INDEXLOG_H_ */

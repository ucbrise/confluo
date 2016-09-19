#ifndef SLOG_NGRAM_IDX_H_
#define SLOG_NGRAM_IDX_H_

#include <map>
#include <unordered_map>

#include "flags.h"
#include "monolog.h"
#include "utils.h"

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

typedef uint64_t index_entry;
typedef monolog_relaxed<index_entry, 24> entry_list;

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

  void add_entry(const unsigned char* token, const uint32_t record_id) {
    uint32_t prefix = token_ops<TOKEN_LEN, PREFIX_LEN>::prefix(token);
    uint64_t suffix = token_ops<TOKEN_LEN, PREFIX_LEN>::suffix(token);
    if (idx_[prefix] == NULL) {
      try_allocate_list(prefix);
    }
    entry_list* list = idx_[prefix];
    index_entry entry = (suffix << 32) | record_id;
    list->push_back(entry);
  }

  entry_list* get_entry_list(const unsigned char* token) {
    uint32_t prefix = token_ops<TOKEN_LEN, PREFIX_LEN>::prefix(token);
    return idx_[prefix];
  }

  entry_list* get_entry_list(uint32_t idx) {
    return idx_[idx];
  }

  size_t serialize(std::ostream& out) {
    size_t out_size = 0;

    // Write n-gram index
    size_t ngram_idx_size = size();
    out.write(reinterpret_cast<const char *>(&(ngram_idx_size)),
              sizeof(size_t));
    out_size += sizeof(size_t);
    {
      for (uint32_t i = 0; i < idx_.size(); i++) {
        entry_list* entry = idx_[i].load();
        if (entry != NULL) {
          out.write(reinterpret_cast<const char *>(&(i)), sizeof(uint32_t));
          out_size += (sizeof(uint32_t));
          out_size += entry->serialize(out);
        }
      }
    }
    return out_size;
  }

  size_t deserialize(std::istream& in) {
    size_t in_size = 0;

    // Read n-gram index
    size_t ngram_idx_size;
    in.read(reinterpret_cast<char *>(&ngram_idx_size), sizeof(size_t));
    in_size += sizeof(size_t);
    for (size_t i = 0; i < ngram_idx_size; i++) {
      uint32_t first;

      in.read(reinterpret_cast<char *>(&(first)), sizeof(uint32_t));
      in_size += sizeof(uint32_t);

      slog::entry_list* offset_list = new slog::entry_list;
      in_size += offset_list->deserialize(in);
      idx_[first] = offset_list;
    }

    return in_size;
  }

  uint64_t storage_size() {
    uint64_t array_size = idx_.size() * sizeof(atomic_ref);
    uint64_t data_size = 0;
    for (uint32_t i = 0; i < idx_.size(); i++) {
      entry_list* entry = idx_[i].load();
      if (entry != NULL) {
        data_size += entry->storage_footprint();
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

#endif /* SLOG_NGRAM_IDX_H_ */

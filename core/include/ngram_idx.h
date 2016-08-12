#ifndef SLOG_NGRAM_IDX_H_
#define SLOG_NGRAM_IDX_H_

#include <map>
#include <unordered_map>

#include "faclog.h"
#include "flags.h"
#include "utils.h"

namespace slog {

typedef faclog_relaxed<int32_t, 24> offset_list;

template<uint32_t MAX_SIZE = 16777216>
class array_ngram_index {
 public:
  typedef std::atomic<offset_list*> atomic_ref;

  array_ngram_index() {
    offset_list* null_ptr = NULL;
    for (auto& x : idx_)
      x = null_ptr;
  }

  void add_offset(const char* ngram, const uint32_t offset) {
    uint32_t hash = hash_utils::simple_hash(ngram);
    if (idx_[hash] == NULL) {
      try_allocate_list(hash);
    }
    offset_list* list = idx_[hash];
    list->push_back(offset);
  }

  offset_list* get_offsets(const char* ngram) {
    uint32_t hash = hash_utils::simple_hash(ngram);
    return idx_[hash];
  }

  size_t serialize(std::ostream& out) {
    size_t out_size = 0;

    // Write n-gram index
    size_t ngram_idx_size = size();
    out.write(reinterpret_cast<const char *>(&(ngram_idx_size)),
              sizeof(size_t));
    out_size += sizeof(size_t);
    {
      for (uint32_t i = 0; i < MAX_SIZE; i++) {
        offset_list* entry = idx_[i];
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

      slog::offset_list* offset_list = new slog::offset_list;
      in_size += offset_list->deserialize(in);
      idx_[first] = offset_list;
    }

    return in_size;
  }

 private:
  void try_allocate_list(uint32_t i) {
    slog::offset_list* new_list = new slog::offset_list;
    slog::offset_list* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_weak(&idx_[i], &null_ptr, new_list))
      return;

    // All other threads will deallocate the newly allocated bucket.
    delete new_list;
  }

  size_t size() {
    size_t sz = 0;
    for (auto& x : idx_)
      if (x != NULL)
        sz++;
    return sz;
  }

  std::array<atomic_ref, MAX_SIZE> idx_;
};

typedef array_ngram_index<> ngram_index;

}

#endif /* SLOG_NGRAM_IDX_H_ */

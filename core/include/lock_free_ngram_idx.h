#ifndef LOCK_FREE_NGRAM_IDX_H_
#define LOCK_FREE_NGRAM_IDX_H_

#include <atomic>
#include <array>
#include "offset_list.h"

/*
 template<uint32_t N, uint32_t ALPHABET_SIZE = 256>
 class LockFreeNGramIdx {
 public:
 OffsetList* operator[](std::string ngram) {
 char *str = (char*) ngram.c_str();
 return Node<N>::Access(&data_, str);
 }

 OffsetList* operator[](char* ngram) {
 return Node<N>::Access(&data_, ngram);
 }

 private:
 template<size_t DEPTH>
 struct Node {
 using ChildType = typename Node<DEPTH - 1>::DataType;
 using DataType = std::atomic<ChildType*>;

 static void TryAllocate(DataType* data) {
 ChildType* exp = NULL;
 ChildType* alloc = new ChildType[ALPHABET_SIZE];
 bool success = std::atomic_compare_exchange_strong(data, &exp, alloc);
 if (!success)
 delete[] alloc;
 }

 static OffsetList* Access(DataType* data, char* ngram) {
 if (data->load() == NULL)
 TryAllocate(data);

 auto arr = data->load();
 return Node<DEPTH - 1>::Access(&arr[*ngram], ngram + 1);
 }
 };

 template<>
 struct Node<0> {
 using ChildType = OffsetList;
 using DataType = std::atomic<ChildType*>;

 static void TryAllocate(DataType* data) {
 ChildType* exp = NULL;
 ChildType* alloc = new ChildType;
 bool success = std::atomic_compare_exchange_strong(data, &exp, alloc);
 if (!success)
 delete alloc;
 }

 static OffsetList* Access(DataType* data, char* ngram) {
 if (data->load() == NULL)
 TryAllocate(data);

 return data->load();
 }
 };

 Node<N>::DataType data_;
 };
 */

typedef std::atomic<OffsetList*> AtomicOLRef;
typedef std::atomic<AtomicOLRef*> AtomicL1Ref;
typedef std::atomic<AtomicL1Ref*> AtomicL2Ref;

class LockFree4GramIdx {
 public:
  OffsetList* get_offsets(const char* ngram) {
#ifdef USE_INT_HASH
    uint16_t hash = ((uint16_t *) ngram)[0];
#else
    std::string hash(ngram, ngram_n_);
#endif
    uint8_t i1 = ((uint8_t*) ngram)[2];
    uint8_t i2 = ((uint8_t*) ngram)[3];
    return idx_[hash][i1][i2];
  }

  void add_offset(const char* ngram, const uint32_t offset) {
#ifdef USE_INT_HASH
    uint16_t hash = ((uint16_t *) ngram)[0];
#else
    std::string hash(ngram, ngram_n_);
#endif
    if (idx_[hash] == NULL) {
      try_allocate_l2(hash);
    }
    uint8_t i1 = ((uint8_t*) ngram)[2];
    if (idx_[hash][i1] == NULL) {
      try_allocate_l1(hash, i1);
    }
    uint8_t i2 = ((uint8_t*) ngram)[3];
    if (idx_[hash][i1][i2] == NULL) {
      try_allocate_list(hash, i1, i2);
    }

    OffsetList* list = idx_[hash][i1][i2];
    list->push_back(offset);
  }

 private:

  void try_allocate_l2(uint32_t i) {
    AtomicL1Ref* new_list = new AtomicL1Ref[256];
    AtomicL1Ref* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_strong(&idx_[i], &null_ptr, new_list))
      return;

    // All other threads will deallocate the newly allocated bucket.
    delete new_list;
  }

  void try_allocate_l1(uint32_t i, uint8_t i1) {
    AtomicOLRef* new_list = new AtomicOLRef[256];
    AtomicOLRef* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_strong(&idx_[i][i1], &null_ptr, new_list))
      return;

    // All other threads will deallocate the newly allocated bucket.
    delete new_list;
  }

  void try_allocate_list(uint32_t i, uint8_t i1, uint8_t i2) {
    OffsetList* new_list = new OffsetList;
    OffsetList* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_strong(&idx_[i][i1][i2], &null_ptr,
                                          new_list))
      return;

    // All other threads will deallocate the newly allocated bucket.
    delete new_list;
  }

  std::array<AtomicL2Ref, 65536> idx_;
};

class LockFree3GramIdx {
 public:
  OffsetList* get_offsets(const char* ngram) {
#ifdef USE_INT_HASH
    uint16_t hash = ((uint16_t *) ngram)[0];
#else
    std::string hash(ngram, ngram_n_);
#endif
    uint8_t i1 = ((uint8_t*) ngram)[2];
    return idx_[hash][i1];
  }

  void add_offset(const char* ngram, const uint32_t offset) {
#ifdef USE_INT_HASH
    uint16_t hash = ((uint16_t *) ngram)[0];
#else
    std::string hash(ngram, ngram_n_);
#endif
    if (idx_[hash] == NULL) {
      try_allocate_l1(hash);
    }
    uint8_t i1 = ((uint8_t*) ngram)[2];
    if (idx_[hash][i1] == NULL) {
      try_allocate_list(hash, i1);
    }

    OffsetList* list = idx_[hash][i1];
    list->push_back(offset);
  }

 private:

  void try_allocate_l1(uint32_t i) {
    AtomicOLRef* new_list = new AtomicOLRef[256];
    AtomicOLRef* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_strong(&idx_[i], &null_ptr, new_list))
      return;

    // All other threads will deallocate the newly allocated bucket.
    delete new_list;
  }

  void try_allocate_list(uint32_t i, uint8_t i1) {
    OffsetList* new_list = new OffsetList;
    OffsetList* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_strong(&idx_[i][i1], &null_ptr, new_list))
      return;

    // All other threads will deallocate the newly allocated bucket.
    delete new_list;
  }

  std::array<AtomicL1Ref, 65536> idx_;
};

class LockFree2GramIdx {
 public:
  OffsetList* get_offsets(const char* ngram) {
#ifdef USE_INT_HASH
    uint16_t hash = ((uint16_t *) ngram)[0];
#else
    std::string hash(ngram, ngram_n_);
#endif
    return idx_[hash];
  }

  void add_offset(const char* ngram, const uint32_t offset) {
#ifdef USE_INT_HASH
    uint16_t hash = ((uint16_t *) ngram)[0];
#else
    std::string hash(ngram, ngram_n_);
#endif
    if (idx_[hash] == NULL) {
      try_allocate_list(hash);
    }
    OffsetList* list = idx_[hash];
    list->push_back(offset);
  }

 private:

  void try_allocate_list(uint32_t i) {
    OffsetList* new_list = new OffsetList;
    OffsetList* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_strong(&idx_[i], &null_ptr, new_list))
      return;

    // All other threads will deallocate the newly allocated bucket.
    delete new_list;
  }

  std::array<AtomicOLRef, 65536> idx_;
};

#ifdef HASH4
typedef LockFree4GramIdx LockFreeNGramIdx;
#else
#ifdef HASH3
typedef LockFree3GramIdx LockFreeNGramIdx;
#else
typedef LockFree2GramIdx LockFreeNGramIdx;
#endif
#endif

#endif /* LOCK_FREE_NGRAM_IDX_H_ */

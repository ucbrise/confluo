#ifndef LOCK_FREE_NGRAM_IDX_H_
#define LOCK_FREE_NGRAM_IDX_H_

#include <atomic>
#include "offset_list.h"

template<uint32_t N, uint32_t ALPHABET_SIZE = 256>
class LockFreeNGramIdx {
 public:
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
      bool success = std::atomic_compare_exchange_weak(data, &exp, alloc);
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
      bool success = std::atomic_compare_exchange_weak(data, &exp, alloc);
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

#endif /* LOCK_FREE_NGRAM_IDX_H_ */

#ifndef DIALOG_DIALOG_ALLOCATOR_H_
#define DIALOG_DIALOG_ALLOCATOR_H_

#include <map>
#include "configuration_params.h"

namespace dialog {

template<class T, size_t BLOCK_SIZE>
class mempool {

 public:
  T* allocate(int num_instances = 1) {
      return new T[num_instances];
  }

  void free(T* ptr) {
    delete[] ptr;
  }

};

class mempools {

 public:
  template<class T, size_t BLOCK_SIZE>
  static T* allocate(int tid, int num_instances = 1) {
    int total_memory = 0;
    for (int i = 0; i < MAX_CONCURRENCY; i++) {
      total_memory += aggregates[i];
    }
    mempool* pool;
    pools.get(BLOCK_SIZE, pool);
    T* alloced =  pool.allocate();
    aggregates[tid] += 1; //block_size;
    return alloced;
  }

  template<class T, size_t BLOCK_SIZE>
  static void free(int tid, T* ptr) {
    mempool* pool;
    pools.get(BLOCK_SIZE, pool);
    pool->free(ptr);
    aggregates[tid] -= sizeof(ptr);
  }

  template<class T, size_t BLOCK_SIZE>
  static void add_pool() {
    pools.put(BLOCK_SIZE, new mempool<T, BLOCK_SIZE>());
  }

 private:
  static const int MAX_MEMORY;
  static const int MAX_CONCURRENCY;
  static int aggregates[MAX_CONCURRENCY];

  template<class T>
  static std::map<size_t, mempool<T>> pools;

};

int mempools::MAX_CONCURRENCY = configuration_params::MAX_CONCURRENCY;
int mempools::MAX_MEMORY = configuration_params::MAX_MEMORY;
mempool mempools::pools = new mempool<std::string, 10>();

}

#endif /* DIALOG_DIALOG_ALLOCATOR_H_ */

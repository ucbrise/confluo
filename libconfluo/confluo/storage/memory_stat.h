#ifndef CONFLUO_STORAGE_MEMPOOL_STAT_H_
#define CONFLUO_STORAGE_MEMPOOL_STAT_H_

#include "atomic.h"

namespace confluo {
namespace storage {

/**
 * The memory stat class. Contains functionality for modifying the amount
 * of memory used.
 */
class memory_stat {

 public:
  /**
   * Initializes memory statistics.
   */
  memory_stat();

  /**
   * Increments the memory used by the specified size
   *
   * @param size The size to increment by in bytes
   */
  void increment(size_t size);

  /**
   * Decrements the memory used by the specified size
   *
   * @param size The size to increment by in bytes
   */
  void decrement(size_t size);

  /**
   * Loads the amount of memory used
   *
   * @return The amount of memory used
   */
  size_t get();

 private:
  atomic::type<size_t> memory_used_;

};

}
}

#endif /* CONFLUO_STORAGE_MEMPOOL_STAT_H_ */

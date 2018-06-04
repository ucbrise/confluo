#ifndef CONFLUO_STORAGE_ALLOCATOR_H_
#define CONFLUO_STORAGE_ALLOCATOR_H_

#include "storage_allocator.h"

namespace confluo {

/** Static storage allocator */
class allocator {
 public:
  static storage::storage_allocator &instance() {
    static storage::storage_allocator alloc;
    return alloc;
  }
};

}

#endif /* CONFLUO_STORAGE_ALLOCATOR_H_ */

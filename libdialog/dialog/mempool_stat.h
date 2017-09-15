#ifndef DIALOG_MEMPOOL_STAT_H_
#define DIALOG_MEMPOOL_STAT_H_

#include "configuration_params.h"
#include "atomic.h"

namespace dialog {

class mempool_stat {

 public:
  mempool_stat() :
    memory_used_(0) {
  }

  inline void increment(size_t size) {
    atomic::faa(&memory_used_, size);
  }

  inline void decrement(size_t size) {
    atomic::faa(&memory_used_, -size);
  }

  inline size_t get() {
    return atomic::load(&memory_used_);
  }

 private:
  atomic::type<size_t> memory_used_;

};

}

#endif /* DIALOG_MEMPOOL_STAT_H_ */

#include "storage/memory_stat.h"

namespace confluo {
namespace storage {

memory_stat::memory_stat()
    : memory_used_(0) {
}

void memory_stat::increment(size_t size) {
  atomic::faa(&memory_used_, size);
}

void memory_stat::decrement(size_t size) {
  atomic::fas(&memory_used_, size);
}

size_t memory_stat::get() {
  return atomic::load(&memory_used_);
}

}
}
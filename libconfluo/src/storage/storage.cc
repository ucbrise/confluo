#include "storage/storage.h"

namespace confluo {
namespace storage {

storage_functions &storage_mode_functions::IN_MEMORY_FNS() {
  static storage_functions fns
      {storage_mode::IN_MEMORY, in_memory::allocate, in_memory::allocate_bucket, in_memory::free_mem,
       in_memory::flush};
  return fns;
}

storage_functions &storage_mode_functions::DURABLE_RELAXED_FNS() {
  static storage_functions fns
      {storage_mode::DURABLE_RELAXED, durable_relaxed::allocate, durable_relaxed::allocate_bucket,
       durable_relaxed::free,
       durable_relaxed::flush};
  return fns;
}

storage_functions &storage_mode_functions::DURABLE_FNS() {
  static storage_functions
      fns{storage_mode::DURABLE, durable::allocate, durable::allocate_bucket, durable::free, durable::flush};
  return fns;
}

storage_functions *storage_mode_functions::STORAGE_FNS() {
  static storage_functions fns[3] =
      {storage_mode_functions::IN_MEMORY_FNS(), storage_mode_functions::DURABLE_RELAXED_FNS(),
       storage_mode_functions::DURABLE_FNS()};
  return fns;
}

}
}
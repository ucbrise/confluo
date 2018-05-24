#include "storage/storage.h"

namespace confluo {
namespace storage {

/** Storage functionality for in memory mode */
storage_functions IN_MEMORY_FNS =
    {storage_mode::IN_MEMORY, in_memory::allocate, in_memory::allocate_bucket, in_memory::free_mem, in_memory::flush};

/** Storage functionality for durable relaxed mode */
storage_functions DURABLE_RELAXED_FNS =
    {storage_mode::DURABLE_RELAXED, durable_relaxed::allocate, durable_relaxed::allocate_bucket, durable_relaxed::free,
     durable_relaxed::flush};

/** Storage functionality for durable mode */
storage_functions
    DURABLE_FNS = {storage_mode::DURABLE, durable::allocate, durable::allocate_bucket, durable::free, durable::flush};

/** Contains the storage functions for all storage modes */
storage_functions STORAGE_FNS[3] = {IN_MEMORY_FNS, DURABLE_RELAXED_FNS, DURABLE_FNS};

}
}
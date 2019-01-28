
#include <rpc_handler_registry.h>

#include "rpc_handler_registry.h"

namespace confluo {
namespace rpc {

std::atomic<uint64_t> rpc_handler_registry::handler_id_{0};

uint64_t rpc_handler_registry::add() {
  return handler_id_.fetch_add(1ULL);
}

void rpc_handler_registry::remove(uint64_t) {
  // Do nothing
}

}
}

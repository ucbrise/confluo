#ifndef CONFLUO_RPC_HANDLER_REGISTRY_H
#define CONFLUO_RPC_HANDLER_REGISTRY_H

#include <atomic>

namespace confluo {
namespace rpc {

/**
 * RPC handler registry
 */
class rpc_handler_registry {
 public:
  /**
   * Add a handler to the registry.
   *
   * @return A unique identifier for the handler
   */
  static uint64_t add();

  static void remove(uint64_t id);

 private:
  /** Current handler id **/
  static std::atomic<uint64_t> handler_id_;
};

}
}

#endif //CONFLUO_RPC_HANDLER_REGISTRY_H

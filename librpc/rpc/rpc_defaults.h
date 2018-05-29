#ifndef RPC_RPC_DEFAULTS_H_
#define RPC_RPC_DEFAULTS_H_

#include <cstddef>

namespace confluo {
namespace rpc {

/**
 * Holder of default values for parameters
 */
class rpc_defaults {
 public:
  // Iterator
  /** Default batch size for the iterator */
  static const size_t DEFAULT_ITERATOR_BATCH_SIZE = 20;
};

}
}

#endif /* RPC_RPC_DEFAULTS_H_ */

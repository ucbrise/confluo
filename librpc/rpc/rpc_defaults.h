#ifndef RPC_RPC_DEFAULTS_H_
#define RPC_RPC_DEFAULTS_H_

namespace confluo {
namespace rpc {

class rpc_defaults {
 public:
  // Iterator
  static const size_t DEFAULT_ITERATOR_BATCH_SIZE = 20;

  // Write batching
  static const size_t DEFAULT_WRITE_BATCH_SIZE = 20;

  // Read batching
  static const size_t DEFAULT_READ_BATCH_SIZE = 128;
};

const size_t rpc_defaults::DEFAULT_ITERATOR_BATCH_SIZE;
const size_t rpc_defaults::DEFAULT_WRITE_BATCH_SIZE;
const size_t rpc_defaults::DEFAULT_READ_BATCH_SIZE;

}
}

#endif /* RPC_RPC_DEFAULTS_H_ */

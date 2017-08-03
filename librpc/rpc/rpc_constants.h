#ifndef RPC_RPC_CONSTANTS_H_
#define RPC_RPC_CONSTANTS_H_

namespace dialog {
namespace rpc {

class rpc_constants {
 public:
  // Iterator
  static const size_t DEFAULT_ITERATOR_BATCH_SIZE = 128;

  // Write batching
  static const size_t DEFAULT_WRITE_BATCH_SIZE = 128;

  // Read batching
  static const size_t DEFAULT_READ_BATCH_SIZE = 128;
};

const size_t rpc_constants::DEFAULT_ITERATOR_BATCH_SIZE;
const size_t rpc_constants::DEFAULT_WRITE_BATCH_SIZE;
const size_t rpc_constants::DEFAULT_READ_BATCH_SIZE;

}
}

#endif /* RPC_RPC_CONSTANTS_H_ */

#ifndef RPC_RPC_DEFAULTS_H_
#define RPC_RPC_DEFAULTS_H_

namespace confluo {
namespace rpc {

class rpc_defaults {
 public:
  // Iterator
  static const size_t DEFAULT_ITERATOR_BATCH_SIZE = 20;
};

const size_t rpc_defaults::DEFAULT_ITERATOR_BATCH_SIZE;

}
}

#endif /* RPC_RPC_DEFAULTS_H_ */

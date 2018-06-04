#ifndef RPC_RPC_CONFIGURATION_PARAMS_H_
#define RPC_RPC_CONFIGURATION_PARAMS_H_

#include <cstddef>
#include <conf/configuration_params.h>
#include "rpc_defaults.h"

namespace confluo {
namespace rpc {

/**
 * Configuration parameters for the rpc client
 */
class rpc_configuration_params {
 public:
  /** Iterator for the batches */
  static size_t ITERATOR_BATCH_SIZE() {
    return conf::instance().get<size_t>("iterator_batch_size", rpc_defaults::DEFAULT_ITERATOR_BATCH_SIZE());
  }
};

}
}

#endif /* RPC_RPC_CONFIGURATION_PARAMS_H_ */

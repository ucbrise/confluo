#ifndef RPC_RPC_CONFIGURATION_PARAMS_H_
#define RPC_RPC_CONFIGURATION_PARAMS_H_

#include "conf/configuration_params.h"
#include "config_utils.h"
#include "rpc_defaults.h"

namespace confluo {
namespace rpc {

class rpc_configuration_params {
 public:
  /** Iterator for the batches */
  static size_t ITERATOR_BATCH_SIZE;
};

size_t rpc_configuration_params::ITERATOR_BATCH_SIZE = confluo_conf.get<size_t>(
    "iterator_batch_size", rpc_defaults::DEFAULT_ITERATOR_BATCH_SIZE);

}
}

#endif /* RPC_RPC_CONFIGURATION_PARAMS_H_ */

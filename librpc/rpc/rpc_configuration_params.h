#ifndef RPC_RPC_CONFIGURATION_PARAMS_H_
#define RPC_RPC_CONFIGURATION_PARAMS_H_

#include "rpc_constants.h"
#include "config_utils.h"
#include "configuration_params.h"

namespace dialog {
namespace rpc {

class rpc_configuration_params {
 public:
  // Iterator
  static size_t ITERATOR_BATCH_SIZE;

  // Write batching
  static size_t WRITE_BATCH_SIZE;

  // Read batching
  static size_t READ_BATCH_SIZE;
};

size_t rpc_configuration_params::ITERATOR_BATCH_SIZE = dialog_conf.get<size_t>(
    "iterator_batch_size", rpc_constants::DEFAULT_ITERATOR_BATCH_SIZE);

size_t rpc_configuration_params::WRITE_BATCH_SIZE = dialog_conf.get<size_t>(
    "write_batch_size", rpc_constants::DEFAULT_WRITE_BATCH_SIZE);

size_t rpc_configuration_params::READ_BATCH_SIZE = dialog_conf.get<size_t>(
    "read_batch_size", rpc_constants::DEFAULT_READ_BATCH_SIZE);

}
}

#endif /* RPC_RPC_CONFIGURATION_PARAMS_H_ */

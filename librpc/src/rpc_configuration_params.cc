#include "rpc_configuration_params.h"
#include "conf/configuration_params.h"
#include "rpc_defaults.h"

namespace confluo {
namespace rpc {

size_t rpc_configuration_params::ITERATOR_BATCH_SIZE =
    confluo_conf.get<size_t>("iterator_batch_size", rpc_defaults::DEFAULT_ITERATOR_BATCH_SIZE);

}
}
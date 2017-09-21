#ifndef DIALOG_DATA_LOG_POOL_H_
#define DIALOG_DATA_LOG_POOL_H_

#include "data_log_constants.h"
#include "mempool.h"

namespace dialog {

static mempool<uint8_t> DATA_LOG_BLOCK_POOL;

}

#endif /* DIALOG_DATA_LOG_POOL_H_ */

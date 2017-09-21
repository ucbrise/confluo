#ifndef DIALOG_DATA_LOG_H_
#define DIALOG_DATA_LOG_H_

#include "data_log_constants.h"
#include "monolog_linear.h"

namespace dialog {

using data_log = monolog::monolog_linear<uint8_t,
    data_log_constants::MAX_BLOCKS,
    data_log_constants::BLOCK_SIZE,
    data_log_constants::BUFFER_SIZE>;

}

#endif /* DIALOG_DATA_LOG_H_ */

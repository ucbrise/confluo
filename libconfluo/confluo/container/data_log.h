#ifndef CONFLUO_CONTAINER_DATA_LOG_H_
#define CONFLUO_CONTAINER_DATA_LOG_H_

#include "monolog/monolog.h"

namespace confluo {

/**
 * Constants for the data log
 */
class data_log_constants {
 public:
  /** Maximum number of blocks */
  static const size_t MAX_BLOCKS = 65536;
  /** The size of each block */
  static const size_t BLOCK_SIZE = 67108864;
  /** The size of the buffer */
  static const size_t BUFFER_SIZE = 1048576;
};

const size_t data_log_constants::MAX_BLOCKS;
const size_t data_log_constants::BLOCK_SIZE;
const size_t data_log_constants::BUFFER_SIZE;

typedef monolog::monolog_linear<uint8_t, data_log_constants::MAX_BLOCKS,
    data_log_constants::BLOCK_SIZE, data_log_constants::BUFFER_SIZE> data_log;

typedef storage::read_only_ptr<uint8_t> ro_data_ptr;

}

#endif /* CONFLUO_CONTAINER_DATA_LOG_H_ */

#ifndef CONFLUO_CONTAINER_DATA_LOG_H_
#define CONFLUO_CONTAINER_DATA_LOG_H_

#include "monolog/monolog.h"

namespace confluo {

class data_log_constants {
 public:
  static constexpr size_t MAX_BLOCKS = 65536;
  static constexpr size_t BLOCK_SIZE = 67108864;
  static constexpr size_t BUFFER_SIZE = 1048576;
};

constexpr size_t data_log_constants::MAX_BLOCKS;
constexpr size_t data_log_constants::BLOCK_SIZE;
constexpr size_t data_log_constants::BUFFER_SIZE;

typedef monolog::monolog_linear<uint8_t, data_log_constants::MAX_BLOCKS,
    data_log_constants::BLOCK_SIZE, data_log_constants::BUFFER_SIZE> data_log;

typedef storage::read_only_ptr<uint8_t> ro_data_ptr;

}

#endif /* CONFLUO_CONTAINER_DATA_LOG_H_ */

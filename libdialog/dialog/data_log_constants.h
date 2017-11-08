#ifndef DIALOG_DATA_LOG_CONSTANTS_H_
#define DIALOG_DATA_LOG_CONSTANTS_H_

namespace dialog {

class data_log_constants {
 public:
  static const size_t MAX_BLOCKS = 65536;
  static const size_t BLOCK_SIZE = 67108864;
  static const size_t BUFFER_SIZE = 1048576;
};

const size_t data_log_constants::MAX_BLOCKS;
const size_t data_log_constants::BLOCK_SIZE;
const size_t data_log_constants::BUFFER_SIZE;

}

#endif /* DIALOG_DATA_LOG_CONSTANTS_H_ */

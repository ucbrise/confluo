#ifndef DIALOG_DATA_LOG_CONSTANTS_H_
#define DIALOG_DATA_LOG_CONSTANTS_H_

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

}

#endif /* DIALOG_DATA_LOG_CONSTANTS_H_ */

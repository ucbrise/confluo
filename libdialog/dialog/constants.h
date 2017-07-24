#ifndef DIALOG_CONSTANTS_H_
#define DIALOG_CONSTANTS_H_

#include <thread>

namespace dialog {

class constants {
 public:
  static const uint32_t HARDWARE_CONCURRENCY;
};

const uint32_t constants::HARDWARE_CONCURRENCY = std::thread::hardware_concurrency();

}



#endif /* DIALOG_CONSTANTS_H_ */

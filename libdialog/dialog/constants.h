#ifndef DIALOG_CONSTANTS_H_
#define DIALOG_CONSTANTS_H_

#include <thread>

namespace dialog {

class constants {
 public:
  static const int HARDWARE_CONCURRENCY;
};

const int constants::HARDWARE_CONCURRENCY = std::thread::hardware_concurrency();

}



#endif /* DIALOG_CONSTANTS_H_ */

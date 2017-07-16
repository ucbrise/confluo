#ifndef DIALOG_DATA_LOG_H_
#define DIALOG_DATA_LOG_H_

#include "monolog_linear.h"

namespace dialog {

template<class sm>
using data_log = monolog::monolog_linear<uint8_t, 65536, 1073741824, 1048576, sm>;

}

#endif /* DIALOG_DATA_LOG_H_ */

#ifndef DIALOG_DATA_LOG_H_
#define DIALOG_DATA_LOG_H_

#include "monolog_linear.h"

namespace dialog {

using data_log = monolog::monolog_linear<uint8_t, 65536, 67108864, 1048576>;

}

#endif /* DIALOG_DATA_LOG_H_ */

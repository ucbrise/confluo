#ifndef DIALOG_FILTER_LOG_H_
#define DIALOG_FILTER_LOG_H_

#include "monolog_exp2.h"
#include "filter.h"

namespace dialog {

typedef monolog::monolog_exp2_linear<monitor::filter*> filter_log;

}

#endif /* DIALOG_FILTER_LOG_H_ */

#ifndef CONFLUO_FILTER_LOG_H_
#define CONFLUO_FILTER_LOG_H_

#include "container/monolog/monolog_exp2.h"
#include "filter.h"

namespace confluo {

typedef monolog::monolog_exp2<monitor::filter*> filter_log;

}

#endif /* CONFLUO_FILTER_LOG_H_ */

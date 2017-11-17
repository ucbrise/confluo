#ifndef CONFLUO_TRIGGER_LOG_H_
#define CONFLUO_TRIGGER_LOG_H_

#include "container/monolog/monolog_exp2.h"
#include "trigger.h"

namespace confluo {

typedef monolog::monolog_exp2<monitor::trigger*> trigger_log;

}

#endif /* CONFLUO_TRIGGER_LOG_H_ */

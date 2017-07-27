#ifndef DIALOG_TRIGGER_LOG_H_
#define DIALOG_TRIGGER_LOG_H_

#include "monolog_exp2.h"
#include "trigger.h"

namespace dialog {

typedef monolog::monolog_exp2<monitor::trigger*> trigger_log;

}

#endif /* DIALOG_TRIGGER_LOG_H_ */

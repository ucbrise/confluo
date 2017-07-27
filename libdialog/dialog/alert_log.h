#ifndef DIALOG_ALERT_LOG_H_
#define DIALOG_ALERT_LOG_H_

#include "monolog.h"
#include "alert.h"

namespace dialog {
namespace monitor {

typedef monolog::monolog_exp2<alert*> alert_log;

}
}

#endif /* DIALOG_ALERT_LOG_H_ */

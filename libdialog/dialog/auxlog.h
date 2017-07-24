#ifndef DIALOG_AUXLOG_H_
#define DIALOG_AUXLOG_H_

#include "monolog.h"
#include "radix_tree.h"
#include "trigger.h"
#include "filter.h"

namespace dialog {

template<typename T>
using aux_log_t = monolog::monolog_exp2<T>;

typedef aux_log_t<index::radix_tree*> index_list_type;
typedef aux_log_t<monitor::trigger*> trigger_list_type;
typedef aux_log_t<monitor::filter*> filter_list_type;

}

#endif /* DIALOG_AUXLOG_H_ */

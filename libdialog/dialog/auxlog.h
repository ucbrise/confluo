#ifndef DIALOG_AUXLOG_H_
#define DIALOG_AUXLOG_H_

#include "monolog.h"
#include "radix_tree.h"
#include "trigger.h"
#include "filter.h"

namespace dialog {

template<typename T, class sm>
using aux_log_t = monolog::monolog_linear<T, 256, 65536, 0, sm>;

typedef aux_log_t<index::radix_tree*, storage::in_memory> index_list_t;
typedef aux_log_t<monitor::trigger*, storage::in_memory> trigger_list_t;
typedef aux_log_t<monitor::filter*, storage::in_memory> filter_list_t;

}

#endif /* DIALOG_AUXLOG_H_ */

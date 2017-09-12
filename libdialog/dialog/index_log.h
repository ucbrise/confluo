#ifndef DIALOG_INDEX_LOG_H_
#define DIALOG_INDEX_LOG_H_

#include "monolog_exp2.h"
#include "radix_tree.h"

namespace dialog {

typedef monolog::monolog_exp2_linear<index::radix_index*> index_log;

}

#endif /* DIALOG_INDEX_LOG_H_ */

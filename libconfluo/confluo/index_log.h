#ifndef CONFLUO_INDEX_LOG_H_
#define CONFLUO_INDEX_LOG_H_

#include "container/monolog/monolog_exp2.h"
#include "container/radix_tree.h"

namespace confluo {

typedef monolog::monolog_exp2<index::radix_index*> index_log;

}

#endif /* CONFLUO_INDEX_LOG_H_ */

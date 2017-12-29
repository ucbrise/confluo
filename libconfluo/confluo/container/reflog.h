#ifndef CONFLUO_CONTAINER_REFLOG_H_
#define CONFLUO_CONTAINER_REFLOG_H_

#include "monolog/monolog.h"

using namespace ::confluo::monolog;

namespace confluo {

/**
 * typedef for RefLog type -- a MonoLog of type uint64_t,
 * 18 bucket containers and bucket size of 1024.
 */
typedef monolog_exp2_linear<uint64_t, 18, 1024> reflog;

}

#endif /* CONFLUO_CONTAINER_REFLOG_H_ */

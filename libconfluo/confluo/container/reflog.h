#ifndef CONFLUO_CONTAINER_REFLOG_H_
#define CONFLUO_CONTAINER_REFLOG_H_

#include "monolog/monolog.h"

using namespace ::confluo::monolog;

namespace confluo {

class reflog_constants {
 public:
  static const size_t NCONTAINERS = 18;
  static const size_t BUCKET_SIZE = 1024;
};

const size_t reflog_constants::NCONTAINERS;
const size_t reflog_constants::BUCKET_SIZE;

/**
 * typedef for RefLog type -- a MonoLog of type uint64_t,
 * 18 bucket containers and bucket size of 1024.
 */
typedef monolog_exp2_linear<uint64_t,
                            reflog_constants::NCONTAINERS,
                            reflog_constants::BUCKET_SIZE> reflog;

}

#endif /* CONFLUO_CONTAINER_REFLOG_H_ */

#ifndef CONFLUO_CONTAINER_REFLOG_H_
#define CONFLUO_CONTAINER_REFLOG_H_

#include "storage/swappable_encoded_ptr.h"
#include "storage/encoded_ptr.h"
#include "monolog/monolog.h"

using namespace ::confluo::monolog;

namespace confluo {

class reflog_constants {
 public:
  static const size_t NCONTAINERS = 18;
  static const size_t BUCKET_SIZE = 1024;
};

/**
 * typedef for RefLog type -- a MonoLog of type uint64_t,
 * 18 bucket containers and bucket size of 1024.
 */
typedef monolog_exp2_linear<uint64_t,
                            reflog_constants::NCONTAINERS,
                            reflog_constants::BUCKET_SIZE> reflog;

typedef storage::read_only_encoded_ptr<uint64_t> read_only_reflog_ptr;
typedef storage::encoded_ptr<uint64_t> encoded_reflog_ptr;
typedef storage::decoded_ptr<uint64_t> decoded_reflog_ptr;

}

#endif /* CONFLUO_CONTAINER_REFLOG_H_ */

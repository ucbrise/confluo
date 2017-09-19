#ifndef LIBDIALOG_DIALOG_REFLOG_H_
#define LIBDIALOG_DIALOG_REFLOG_H_

#include "mempool.h"
#include "monolog.h"

using namespace ::dialog::monolog;

namespace dialog {

static constexpr size_t REFLOG_BUCKET_SIZE = 1024;
static constexpr int NBC = 18;
static constexpr size_t REFLOG_BLOCK_SIZE = REFLOG_BUCKET_SIZE * sizeof(uint64_t);

/**
 * RefLog type -- a MonoLog of type uint64_t,
 * 18 bucket containers and bucket size of 1024.
 */
class reflog : public monolog_exp2_linear<uint64_t, NBC, REFLOG_BUCKET_SIZE> {

 public:
  reflog() :
    monolog_exp2_linear<uint64_t, NBC, REFLOG_BUCKET_SIZE>(REFLOG_BUCKET_POOL) {
  }

 private:
  static mempool<uint64_t, REFLOG_BLOCK_SIZE> REFLOG_BUCKET_POOL;

};

mempool<uint64_t, REFLOG_BLOCK_SIZE> reflog::REFLOG_BUCKET_POOL;

}

#endif /* LIBDIALOG_DIALOG_REFLOG_H_ */

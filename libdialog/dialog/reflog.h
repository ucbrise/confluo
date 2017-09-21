#ifndef LIBDIALOG_DIALOG_REFLOG_H_
#define LIBDIALOG_DIALOG_REFLOG_H_

#include "mempool.h"
#include "monolog.h"

using namespace ::dialog::monolog;

namespace dialog {

/**
 * typedef for RefLog type -- a MonoLog of type uint64_t,
 * 18 bucket containers and bucket size of 1024.
 */
typedef monolog_exp2_linear<uint64_t, 18, 1024> reflog;

}

#endif /* LIBDIALOG_DIALOG_REFLOG_H_ */

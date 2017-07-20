#ifndef LIBDIALOG_DIALOG_REFLOG_H_
#define LIBDIALOG_DIALOG_REFLOG_H_

#include "monolog.h"

namespace dialog {

/**
 * Type-definition for RefLog type -- a MonoLog of type uint64_t and
 * bucket size of 24.
 */
typedef monolog::monolog_exp2<uint64_t, 24> reflog;

}


#endif /* LIBDIALOG_DIALOG_REFLOG_H_ */

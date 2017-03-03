#ifndef SLOG_ENTRYLIST_H_
#define SLOG_ENTRYLIST_H_

#include "monolog.h"

namespace slog {

typedef monolog_relaxed<uint64_t, 24> entry_list;

}

#endif /* SLOG_ENTRYLIST_H_ */

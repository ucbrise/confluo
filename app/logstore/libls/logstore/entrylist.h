#ifndef MONOLOG_ENTRYLIST_H_
#define MONOLOG_ENTRYLIST_H_

#include "monolog.h"

namespace monolog {

typedef monolog_relaxed<uint64_t, 24> entry_list;

}

#endif /* MONOLOG_ENTRYLIST_H_ */

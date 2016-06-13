#ifndef OFFSET_LIST_H_
#define OFFSET_LIST_H_

#include "flags.h"
#include "conc_vectors.h"

#ifdef LOCK_FREE_OFFSET_LIST
typedef LockFreeGrowingList<int32_t, 24> OffsetList;
#else
typedef ConcurrentVector<uint32_t> OffsetList;
#endif

#endif /* OFFSET_LIST_H_ */

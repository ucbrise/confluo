#ifndef OFFSET_LIST_H_
#define OFFSET_LIST_H_

#include "flags.h"
#include "conc_vectors.h"

#ifdef LOCK_FREE
typedef LockFreeGrowingList<int32_t,64,24,-1> OffsetList;
#else
typedef ConcurrentVector<uint32_t> OffsetList;
#endif

#endif /* OFFSET_LIST_H_ */

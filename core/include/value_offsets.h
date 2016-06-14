#ifndef VALUE_OFFSETS_H_
#define VALUE_OFFSETS_H_

#include "conc_vectors.h"

typedef __LockFreeBase<uint32_t, 32> ValueOffsets;
typedef __LockFreeBaseAtomic<uint32_t, 32> DeletedOffsets;

#endif /* VALUE_OFFSETS_H_ */

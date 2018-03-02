#ifndef CONFLUO_SCHEMA_COLUMN_SNAPSHOT_H_
#define CONFLUO_SCHEMA_COLUMN_SNAPSHOT_H_

#include <cstdint>

#include "types/data_type.h"

namespace confluo {

/**
 * Data for snapshot of column
 */
struct column_snapshot {
  /** The data type of the column */
  data_type type;
  /** The offset of the column */
  size_t offset;
  /** Whether the column is indexed */
  bool indexed;
  /** The identifier for the index */
  uint32_t index_id;
  /** The bucket size for the index */
  double index_bucket_size;
};

}

#endif /* CONFLUO_SCHEMA_COLUMN_SNAPSHOT_H_ */

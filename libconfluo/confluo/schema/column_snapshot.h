#ifndef CONFLUO_SCHEMA_COLUMN_SNAPSHOT_H_
#define CONFLUO_SCHEMA_COLUMN_SNAPSHOT_H_

#include <cstdint>

#include "types/data_type.h"

namespace confluo {

    /**
     * @brief Data for snapshot of column
     */
struct column_snapshot {
  data_type type;
  size_t offset;
  bool indexed;
  uint32_t index_id;
  double index_bucket_size;
};

}

#endif /* CONFLUO_SCHEMA_COLUMN_SNAPSHOT_H_ */

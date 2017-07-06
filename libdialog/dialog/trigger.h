#ifndef DIALOG_TRIGGER_H_
#define DIALOG_TRIGGER_H_

#include <cstdint>

#include "schema.h"

namespace dialog {
namespace monitor {

enum aggregate_t {
  COUNT = 0,
  SUM = 1,
  AVERAGE = 2,
  MIN = 3,
  MAX = 4,
  UDF = 5
};

struct trigger_info {
  uint32_t trigger_id;
  uint32_t filter_id;
  column_t col;
  aggregate_t agg_type;
};

}
}

#endif /* DIALOG_TRIGGER_H_ */

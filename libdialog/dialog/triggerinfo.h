#ifndef DIALOG_TRIGGER_H_
#define DIALOG_TRIGGER_H_

#include <cstdint>

#include "attributes.h"

namespace dialog {
namespace trigger {

enum aggregate_type {
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
  attribute attr;
  aggregate_type agg_type;
};

}
}

#endif /* DIALOG_TRIGGER_H_ */

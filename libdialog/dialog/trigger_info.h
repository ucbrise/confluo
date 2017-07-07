#ifndef DIALOG_TRIGGER_H_
#define DIALOG_TRIGGER_H_

#include <cstdint>

#include "schema.h"
#include "aggregate.h"

namespace dialog {
namespace monitor {

struct trigger_info {
  uint32_t trigger_id;
  uint32_t filter_id;
  column_t col;
  aggregate_id agg_type;
};

}
}

#endif /* DIALOG_TRIGGER_H_ */

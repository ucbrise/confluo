#ifndef DIALOG_AGGREGATE_TYPES_H_
#define DIALOG_AGGREGATE_TYPES_H_

#include "exceptions.h"

namespace dialog {

enum aggregate_id
  : uint8_t {
    D_SUM = 0,
  D_MIN = 1,
  D_MAX = 2,
  D_CNT = 3
};

class aggop_utils {
 public:
  static std::string agg_to_string(aggregate_id agg) {
    switch (agg) {
      case aggregate_id::D_SUM:
        return "SUM";
      case aggregate_id::D_MIN:
        return "MIN";
      case aggregate_id::D_MAX:
        return "MAX";
      case aggregate_id::D_CNT:
        return "CNT";
      default:
        return "**INVALID**";
    }
  }

  static aggregate_id string_to_agg(const std::string& str) {
    std::string test = utils::string_utils::to_upper(str);
    if (test == "SUM") {
      return aggregate_id::D_SUM;
    } else if (test == "MIN") {
      return aggregate_id::D_MIN;
    } else if (test == "MAX") {
      return aggregate_id::D_MAX;
    } else if (test == "CNT") {
      return aggregate_id::D_CNT;
    } else {
      THROW(parse_exception, "Invalid aggregate function " + str);
    }
  }
};

}

#endif /* DIALOG_AGGREGATE_TYPES_H_ */

#ifndef CONFLUO_TYPES_AGGREGATE_TYPES_H_
#define CONFLUO_TYPES_AGGREGATE_TYPES_H_

#include "exceptions.h"
#include "string_utils.h"

namespace confluo {

enum aggregate_type
  : uint8_t {
    D_SUM = 0,
  D_MIN = 1,
  D_MAX = 2,
  D_CNT = 3
};

class aggregate_type_utils {
 public:
  static std::string agg_to_string(aggregate_type agg) {
    switch (agg) {
      case aggregate_type::D_SUM:
        return "SUM";
      case aggregate_type::D_MIN:
        return "MIN";
      case aggregate_type::D_MAX:
        return "MAX";
      case aggregate_type::D_CNT:
        return "CNT";
      default:
        return "**INVALID**";
    }
  }

  static aggregate_type string_to_agg(const std::string& str) {
    std::string test = utils::string_utils::to_upper(str);
    if (test == "SUM") {
      return aggregate_type::D_SUM;
    } else if (test == "MIN") {
      return aggregate_type::D_MIN;
    } else if (test == "MAX") {
      return aggregate_type::D_MAX;
    } else if (test == "CNT") {
      return aggregate_type::D_CNT;
    } else {
      THROW(parse_exception, "Invalid aggregate function " + str);
    }
  }
};

}

#endif /* CONFLUO_TYPES_AGGREGATE_TYPES_H_ */

#ifndef CONFLUO_TYPES_AGGREGATE_TYPES_H_
#define CONFLUO_TYPES_AGGREGATE_TYPES_H_

#include "exceptions.h"
#include "string_utils.h"

namespace confluo {

/**
 * List of different aggregate operations
 */
enum aggregate_type
  : uint8_t {
    /** The sum aggregator */
    D_SUM = 0,
  /** The min aggregator */
  D_MIN = 1,
  /** The max aggregator */
  D_MAX = 2,
  /** The count aggregator */
  D_CNT = 3
};

/**
 * Helper oeprations for aggregates
 */
class aggregate_type_utils {
 public:
  /**
   * Converts aggregate type to a string representation
   * @param agg An enumerated value representing type of aggregate
   * @return A string representation of the aggregate type
   */
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

  /**
   * Converts a string representation of an aggregate_type to an
   * enumerated aggregate_type value
   * @param str The string containing the type of the aggregate
   * @return An aggregate_type containing the appropriate type
   */
  static aggregate_type string_to_agg(const std::string& str) {
    std::string test = utils::string_utils::to_upper(str);
    if (test == "SUM") {
      return aggregate_type::D_SUM;
    } else if (test == "MIN") {
      return aggregate_type::D_MIN;
    } else if (test == "MAX") {
      return aggregate_type::D_MAX;
    } else if (test == "CNT" || test == "COUNT") {
      return aggregate_type::D_CNT;
    } else {
      THROW(parse_exception, "Invalid aggregate function " + str);
    }
  }
};

}

#endif /* CONFLUO_TYPES_AGGREGATE_TYPES_H_ */

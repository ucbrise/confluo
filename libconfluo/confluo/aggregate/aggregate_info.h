#ifndef CONFLUO_AGGREGATE_INFO_H_
#define CONFLUO_AGGREGATE_INFO_H_

#include <utility>
#include <utility>
#include "types/data_type.h"
#include "parser/aggregate_parser.h"
#include "trigger_log.h"

using namespace confluo::monitor;

namespace confluo {

/**
* Information about the aggregate
*/
class aggregate_info {
 public:
  /**
   * Constructor to initialize aggregate_info
   *
   * @param name The name of the aggregate.
   * @param agg The aggregator.
   * @param field_idx Index of field in schema.
   */
  aggregate_info(std::string name, aggregator agg, uint16_t field_idx);

  /**
   * Get the aggregate name
   * @return Aggregate name.
   */
  const std::string name() const;

  /**
   * Gets the result data type for the aggregate.
   * @return The result data type for the aggregate.
   */
  data_type result_type() const;

  /**
   * Gets the field index
   * @return The field index
   */
  uint16_t field_idx() const;

  /**
   * Parses a value with the aggregate data type.
   * @param str String to parse from.
   * @return Numeric value with the aggregate's data type.
   */
  numeric value(const std::string &str);

  /**
   * Aggregates the data using the combine operator.
   * @param a The numeric
   * @param s The snapshot of the schema
   * @param data The data
   * @return An aggregate of the data based on the numeric
   */
  numeric comb_op(const numeric &a, const schema_snapshot &s, void *data);

  /**
   * Aggregates the data using the combine operator.
   * @param a A numeric
   * @param b Another numeric
   * @return An aggregate of the two numerics
   */
  numeric comb_op(const numeric &a, const numeric &b);

  /**
   * Aggregates the data using the combine operator.
   * @param a The numeric
   * @param s The snapshot of the schema
   * @param data The data
   * @return An aggregate of the data based on the numeric
   */
  numeric seq_op(const numeric &a, const schema_snapshot &s, void *data);

  /**
   * Aggregates the data using the combine operator.
   * @param a A numeric
   * @param b Another numeric
   * @return An aggregate of the two numerics
   */
  numeric seq_op(const numeric &a, const numeric &b);

  /**
   * Returns the zero value for the aggregate
   * @return Zero value for the aggregate
   */
  numeric zero();

  /**
   * Creates an aggregate using the parsed aggregate information
   *
   * @return The aggregate
   */
  aggregate create_aggregate() const;

  /**
   * Invalidates the aggregate
   * @return Whether aggregate was successfully invalidated
   */
  bool invalidate();

  /**
   * Determines whether the aggregate is valid
   * @return Whether the aggregate is valid
   */
  bool is_valid() const;

  /**
   * Add a trigger on the aggregate.
   * @param t The trigger to be added.
   * @return Trigger ID.
   */
  size_t add_trigger(trigger *t);

  /**
   * Remove a trigger from the aggregate.
   * @param id The trigger ID.
   * @return True if the trigger was successfully removed, false otherwise.
   */
  bool remove_trigger(size_t id);

  /**
   * Get the trigger corresponding to a given id.
   * @param id The trigger ID.
   * @return The trigger corresponding to the id.
   */
  trigger *get_trigger(size_t id);

  /**
   * Get the number of triggers on this aggregate (includes invalid triggers)
   * @return The number of triggers on this aggregate.
   */
  size_t num_triggers() const;

 private:
  std::string name_;

  // Parsed information
  aggregator agg_;
  uint16_t field_idx_;

  // Other metadata
  trigger_log triggers_;
  atomic::type<bool> is_valid_;
};

/**
 * @brief Log of aggregate_info pointers
 */
typedef monolog::monolog_exp2<aggregate_info *> aggregate_log;

}

#endif /* CONFLUO_AGGREGATE_INFO_H_ */

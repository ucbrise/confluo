#ifndef CONFLUO_AGGREGATE_H_
#define CONFLUO_AGGREGATE_H_

#include <utility>
#include "atomic.h"
#include "threads/thread_manager.h"
#include "aggregate_ops.h"
#include "aggregate_manager.h"
#include "types/numeric.h"
#include "threads/thread_manager.h"

namespace confluo {

class aggregate;

/**
 * A node containing data about the aggregate 
 */
struct aggregate_node {

  /**
   * Default constructor
   */
  aggregate_node();

  /**
   * Constructor for an aggregate_node
   * @param agg The numeric containing the aggregate
   * @param version The version of the aggregate
   * @param next A pointer to the next aggregate
   */
  aggregate_node(numeric agg, uint64_t version, aggregate_node *next);

  /** 
   * @return The value of the aggregate
   */
  numeric value() const;

  /**
   * @return The current version of the aggregate
   */
  uint64_t version() const;

  /**
   * @return A pointer to the next aggregate
   */
  aggregate_node *next();

 private:
  numeric value_;
  uint64_t version_;
  aggregate_node *next_;
};

/**
 * List of aggregates
 */
class aggregate_list {
 public:
  /**
   * Default constructor that initializes an empty list of aggregates
   */
  aggregate_list();

  /**
   * Constructor that initializes an aggregate list with one aggregator
   *
   * @param type The type of the aggregate
   * @param agg The aggregator that is added to the list
   * 
   */
  aggregate_list(data_type type, aggregator agg);

  /**
   * Copy constructor that copies all nodes in the list
   * Note: not thread-safe
   * @param other other aggregate_list
   */
  aggregate_list(const aggregate_list &other);

  /**
   * Assignment operator that copies all nodes in the list
   * Note: not thread-safe
   * @param other other aggregate_list
   */
  aggregate_list &operator=(const aggregate_list &other);

  /**
   * Default destructor.
   */
  ~aggregate_list();

  /**
   * Initializes the type and aggregate of the list
   * @param type The data type of the aggregate
   * @param agg The aggregator
   */
  void init(data_type type, const aggregator &agg) {
    type_ = type;
    agg_ = agg;
  }

  /**
   * Gets the 0th aggregate given the type of the aggregate
   * @return The 0th aggregate
   */
  numeric zero() const {
    return agg_.zero;
  }

  /** 
   * Get the aggregate value corresponding to the given version.
   *
   * @param version The version of the data store.
   * @return The aggregate value.
   */
  numeric get(uint64_t version) const;

  /**
   * Update the aggregate value with given version, using the combine operator.
   *
   * @param value The value with which the aggregate is to be updated.
   * @param version The aggregate version.
   */
  void comb_update(const numeric &value, uint64_t version);

  /**
   * Update the aggregate value with the given version, using the sequential operator.
   *
   * @param value The value with which the aggregate is to be updated.
   * @param version The aggregate version.
   */
  void seq_update(const numeric &value, uint64_t version);

 private:
  /**
   * Get node with largest version smaller than or equal to version.
   *
   * @param version The expected version for the node being searched for.
   * @return The node that satisfies the constraints above (if any), nullptr otherwise.
   */
  aggregate_node *get_node(aggregate_node *head, uint64_t version) const;

  atomic::type<aggregate_node *> head_;
  aggregator agg_;
  data_type type_;
};

/**
 * A summary of the data
 */
class aggregate {
 public:
  /**
   * Initializes an empty aggregate for a none type
   */
  aggregate();

  /**
   * Initializes an aggregate based on the data type and other aggregate
   *
   * @param type The type of the aggregate
   * @param agg The aggregate to initialize
   * @param concurrency Max number of threads to run
   */
  aggregate(const data_type &type, aggregator agg, int concurrency = thread_manager::get_max_concurrency());

  /**
   * Initializes an aggregate from another aggregate
   *
   * @param other The other aggregate used to initialize this aggregate
   */
  aggregate(const aggregate &other);

  /**
   * Assigns another aggregate to this aggregate
   *
   * @param other The other aggregate used to initialize this aggregate
   *
   * @return This updated aggregate
   */
  aggregate &operator=(const aggregate &other);

  /**
   * Moves the other aggregate to this aggregate
   *
   * @param other The other r value aggregate
   */
  aggregate(aggregate &&other) noexcept;

  /**
   * Assigns another aggregate to this aggregate using move semantics
   *
   * @param other The other aggregate to move to this aggregate
   *
   * @return This updated aggregate
   */
  aggregate &operator=(aggregate &&other) noexcept;

  /**
   * Deallocates the aggregate
   */
  ~aggregate();

  /**
   * Sequentially updates the aggregate for a thread
   *
   * @param thread_id The identifier for the thread
   * @param value The value to update to
   * @param version The version of the multilog
   */
  void seq_update(int thread_id, const numeric &value, uint64_t version);

  /**
   * A combinational update of an aggregate for a thread
   *
   * @param thread_id The identifier for a thread
   * @param value The value of the numeric
   * @param version The version of the multilog
   */
  void comb_update(int thread_id, const numeric &value, uint64_t version);

  /**
   * Gets the aggregate at the specified version
   *
   * @param version The version to get the aggregate at
   *
   * @return Numeric representing the aggregated value
   */
  numeric get(uint64_t version) const;

 private:
  data_type type_;
  aggregator agg_;
  aggregate_list *aggs_;
  int concurrency_;
};

}

#endif /* CONFLUO_AGGREGATE_H_ */

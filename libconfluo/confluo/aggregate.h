#ifndef CONFLUO_AGGREGATE_H_
#define CONFLUO_AGGREGATE_H_

#include "atomic.h"
#include "threads/thread_manager.h"
#include "types/aggregate_ops.h"
#include "types/numeric.h"

namespace confluo {

class aggregate;

/** A vector of the aggregators supported */
static std::vector<aggregator> aggregators { sum_aggregator, min_aggregator,
    max_aggregator, count_aggregator };

/**
 * Node that encapsulates the value, version, and the next aggregate
 */
struct aggregate_node {

  /**
   * Constructor for an aggregate_node
   * @param agg The numeric containing the aggregate
   * @param version The version of the aggregate
   * @param next A pointer to the next aggregate
   */
  aggregate_node(numeric agg, uint64_t version, aggregate_node *next)
      : value_(agg),
        version_(version),
        next_(next) {
  }

  /** 
   * @return The value of the aggregate
   */
  inline numeric value() const {
    return value_;
  }

  /**
   * @return The current version of the aggregate
   */
  inline uint64_t version() const {
    return version_;
  }

  /**
   * @return A pointer to the next aggregate
   */
  inline aggregate_node* next() {
    return next_;
  }

 private:
  numeric value_;
  uint64_t version_;
  aggregate_node* next_;
};

class aggregate_list {
 public:
  /**
   * Default constructor that initializes an empty list of aggregates
   */
  aggregate_list()
      : head_(nullptr),
        agg_(invalid_aggregator),
        type_(NONE_TYPE) {

  }

  /**
   * Constructor that initializes an aggregate list with one aggregator
   *
   * @param type The type of the aggregate
   * @param agg The aggregator that is added to the list
   * 
   */
  aggregate_list(data_type type, aggregator agg)
      : head_(nullptr),
        agg_(agg),
        type_(type) {
  }

  /**
   * Constructor that initializes an aggregate list with the type of 
   * aggregate
   *
   * @param type The data type of the aggregate
   * @param atype The aggregate type
   */
  aggregate_list(data_type type, aggregate_type atype)
      : head_(nullptr),
        agg_(aggregators[atype]),
        type_(type) {
  }

  /**
   * Default destructor.
   */
  ~aggregate_list() = default;

  /**
   * Initializes the type and aggregate of the list
   * @param type The data type of the aggregate
   * @param agg The aggregator
   */
  void init(data_type type, aggregator agg) {
    type_ = type;
    agg_ = agg;
  }

  /**
   * Initializes the type of Aggregate and the id
   * @param type The type of the aggregate
   * @param id The Aggregate id
   */
  void init(data_type type, aggregate_type atype) {
    type_ = type;
    agg_ = aggregators[atype];
  }

  /**
   * Gets the 0th aggregate given the type of the aggregate
   * @return The 0th aggregate
   */
  numeric zero() const {
    return agg_.zero(type_);
  }

  /** 
   * Get the aggregate value corresponding to the given version.
   *
   * @param version The version of the data store.
   * @return The aggregate value.
   */
  numeric get(uint64_t version) const {
    aggregate_node *cur_head = atomic::load(&head_);
    aggregate_node *req = get_node(cur_head, version);
    if (req != nullptr)
      return req->value();
    return agg_.zero(type_);
  }

  /**
   * Get update the aggregate value with given version.
   *
   * @param value The value with which the aggregate is to be updated.
   * @param version The aggregate version.
   */
  void update(const numeric& value, uint64_t version) {
    aggregate_node *cur_head = atomic::load(&head_);
    aggregate_node *req = get_node(cur_head, version);
    numeric old_agg = (req == nullptr) ? agg_.zero(type_) : req->value();
    aggregate_node *node = new aggregate_node(agg_.agg(old_agg, value), version,
                                              cur_head);
    atomic::store(&head_, node);
  }

 private:
  /**
   * Get node with largest version smaller than or equal to version.
   *
   * @param version The expected version for the node being searched for.
   * @return The node that satisfies the constraints above (if any), nullptr otherwise.
   */
  aggregate_node* get_node(aggregate_node *head, uint64_t version) const {
    if (head == nullptr)
      return nullptr;

    aggregate_node *node = head;
    aggregate_node *ret = nullptr;
    uint64_t max_version = 0;
    while (node != nullptr) {
      if (node->version() == version)
        return node;

      if (node->version() < version && node->version() > max_version) {
        ret = node;
        max_version = node->version();
      }

      node = node->next();
    }
    return ret;
  }

  atomic::type<aggregate_node*> head_;
  aggregator agg_;
  data_type type_;
};

class aggregate {
 public:
  /**
   * 
   */
  aggregate()
      : type_(NONE_TYPE),
        agg_(invalid_aggregator),
        aggs_(nullptr) {
  }

  aggregate(const data_type& type, aggregate_type atype)
      : type_(type),
        agg_(aggregators[atype]),
        aggs_(new aggregate_list[thread_manager::get_max_concurrency()]) {
    for (int i = 0; i < thread_manager::get_max_concurrency(); i++)
      aggs_[i].init(type, atype);
  }

  void update(int thread_id, const numeric& value, uint64_t version) {
    aggs_[thread_id].update(value, version);
  }

  numeric get(uint64_t version) const {
    numeric val = agg_.zero(type_);
    for (int i = 0; i < thread_manager::get_max_concurrency(); i++)
      val = agg_.agg(val, aggs_[i].get(version));
    return val;
  }

 private:
  data_type type_;
  aggregator agg_;
  aggregate_list* aggs_;
};

}

#endif /* CONFLUO_AGGREGATE_H_ */

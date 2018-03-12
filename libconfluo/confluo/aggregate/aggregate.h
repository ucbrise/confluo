#ifndef CONFLUO_AGGREGATE_H_
#define CONFLUO_AGGREGATE_H_

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

  aggregate_node()
      : aggregate_node(NONE_TYPE, 0, nullptr) {
  }

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

/**
 * List of aggregates
 */
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
  aggregate_list(data_type type, const aggregator& agg)
      : head_(nullptr),
        agg_(agg),
        type_(type) {
  }

  /**
   * Copy constructor that copies all nodes in the list
   * Note: not thread-safe
   * @param other other aggregate_list
   */
  aggregate_list(const aggregate_list& other)
      : head_(nullptr),
        agg_(other.agg_),
        type_(other.type_) {
    aggregate_node* other_tail = atomic::load(&other.head_);
    while (other_tail != nullptr) {
      aggregate_node* cur_head = atomic::load(&head_);
      aggregate_node* new_head = new aggregate_node(other_tail->value(), other_tail->version(), cur_head);
      atomic::store(&head_, new_head);
      other_tail = other_tail->next();
    }
  }

  /**
   * Assignment operator that copies all nodes in the list
   * Note: not thread-safe
   * @param other other aggregate_list
   */
  aggregate_list& operator=(const aggregate_list& other) {
    head_ = nullptr;
    agg_ = other.agg_;
    type_ = other.type_;
    aggregate_node* other_tail = atomic::load(&other.head_);
    while (other_tail != nullptr) {
      aggregate_node* cur_head = atomic::load(&head_);
      aggregate_node* new_head = new aggregate_node(other_tail->value(), other_tail->version(), cur_head);
      atomic::store(&head_, new_head);
      other_tail = other_tail->next();
    }
    return *this;
  }

  /**
   * Default destructor.
   */
  ~aggregate_list() {
    aggregate_node* cur_node = atomic::load(&head_);
    while (cur_node != nullptr) {
      aggregate_node* next = cur_node->next();
      delete cur_node;
      cur_node = next;
    }
  }

  /**
   * Initializes the type and aggregate of the list
   * @param type The data type of the aggregate
   * @param agg The aggregator
   */
  void init(data_type type, const aggregator& agg) {
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
  numeric get(uint64_t version) const {
    aggregate_node *cur_head = atomic::load(&head_);
    aggregate_node *req = get_node(cur_head, version);
    if (req != nullptr)
      return req->value();
    return agg_.zero;
  }

  /**
   * Update the aggregate value with given version, using the combine operator.
   *
   * @param value The value with which the aggregate is to be updated.
   * @param version The aggregate version.
   */
  void comb_update(const numeric& value, uint64_t version) {
    aggregate_node *cur_head = atomic::load(&head_);
    aggregate_node *req = get_node(cur_head, version);
    numeric old_agg = (req == nullptr) ? agg_.zero : req->value();
    aggregate_node *node = new aggregate_node(agg_.comb_op(old_agg, value),
                                              version, cur_head);
    atomic::store(&head_, node);
  }

  /**
   * Update the aggregate value with the given version, using the sequential operator.
   *
   * @param value The value with which the aggregate is to be updated.
   * @param version The aggregate version.
   */
  void seq_update(const numeric& value, uint64_t version) {
    aggregate_node *cur_head = atomic::load(&head_);
    aggregate_node *req = get_node(cur_head, version);
    numeric old_agg = (req == nullptr) ? agg_.zero : req->value();
    aggregate_node *node = new aggregate_node(agg_.seq_op(old_agg, value), version, cur_head);
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

/**
 * A summary of the data
 */
class aggregate {
 public:
  /**
   * Initializes an empty aggregate for a none type
   */
  aggregate()
      : type_(NONE_TYPE),
        agg_(invalid_aggregator),
        aggs_(nullptr),
        concurrency_(0) {
  }

  /**
   * Initializes an aggregate based on the data type and other aggregate
   *
   * @param type The type of the aggregate
   * @param agg The aggregate to initialize
   * @param concurrency Max number of threads to run
   */
  aggregate(const data_type& type, const aggregator& agg,
            int concurrency = thread_manager::get_max_concurrency())
      : type_(type),
        agg_(agg),
        aggs_(new aggregate_list[concurrency]),
        concurrency_(concurrency) {
    for (int i = 0; i < concurrency_; i++)
      aggs_[i].init(type, agg_);
  }

  /**
   * Initializes an aggregate from another aggregate
   *
   * @param other The other aggregate used to initialize this aggregate
   */
  aggregate(const aggregate& other)
      : type_(other.type_),
        agg_(other.agg_),
        aggs_(new aggregate_list[other.concurrency_]),
        concurrency_(other.concurrency_) {
    for (int i = 0; i < other.concurrency_; i++) {
      aggs_[i] = other.aggs_[i];
    }
  }

  /**
   * Assigns another aggregate to this aggregate
   *
   * @param other The other aggregate used to initialize this aggregate
   *
   * @return This updated aggregate
   */
  aggregate& operator=(const aggregate& other) {
    type_ = other.type_;
    agg_ = other.agg_;
    aggs_ = new aggregate_list[other.concurrency_];
    concurrency_ = other.concurrency_;
    for (int i = 0; i < other.concurrency_; i++) {
      aggs_[i] = other.aggs_[i];
    }
    return *this;
  }

  /**
   * Moves the other aggregate to this aggregate
   *
   * @param other The other r value aggregate
   */
  aggregate(aggregate&& other) {
    type_ = std::move(other.type_);
    agg_= std::move(other.agg_);
    aggs_ = std::move(other.aggs_);
    concurrency_ = std::move(other.concurrency_);
    other.aggs_ = nullptr;
  }

  /**
   * Assigns another aggregate to this aggregate using move semantics
   *
   * @param other The other aggregate to move to this aggregate
   *
   * @return This updated aggregate
   */
  aggregate& operator=(aggregate&& other) {
    type_ = std::move(other.type_);
    agg_= std::move(other.agg_);
    aggs_ = std::move(other.aggs_);
    concurrency_ = std::move(other.concurrency_);
    other.aggs_ = nullptr;
    return *this;
  }

  /**
   * Deallocates the aggregate
   */
  ~aggregate() {
    if (aggs_ != nullptr) {
      delete[] aggs_;
    }
  }

  /**
   * Sequentially updates the aggregate for a thread
   *
   * @param thread_id The identifier for the thread
   * @param value The value to update to
   * @param version The version of the multilog
   */
  void seq_update(int thread_id, const numeric& value, uint64_t version) {
    aggs_[thread_id].seq_update(value, version);
  }

  /**
   * A combinational update of an aggregate for a thread
   *
   * @param thread_id The identifier for a thread
   * @param value The value of the numeric
   * @param version The version of the multilog
   */
  void comb_update(int thread_id, const numeric& value, uint64_t version) {
    aggs_[thread_id].comb_update(value, version);
  }

  /**
   * Gets the aggregate at the specified version
   *
   * @param version The version to get the aggregate at
   *
   * @return Numeric representing the aggregated value
   */
  numeric get(uint64_t version) const {
    numeric val = agg_.zero;
    for (int i = 0; i < concurrency_; i++)
      val = agg_.comb_op(val, aggs_[i].get(version));
    return val;
  }

 private:
  data_type type_;
  aggregator agg_;
  aggregate_list* aggs_;
  int concurrency_;
};

}

#endif /* CONFLUO_AGGREGATE_H_ */

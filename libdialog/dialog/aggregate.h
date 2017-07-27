#ifndef DIALOG_AGGREGATE_H_
#define DIALOG_AGGREGATE_H_

#include "atomic.h"
#include "numeric.h"
#include "thread_manager.h"

namespace dialog {

enum aggregate_id
  : uint8_t {
    D_SUM = 0,
  D_MIN = 1,
  D_MAX = 2,
  D_CNT = 3
};

class aggregate_ops {
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
    if (str == "SUM") {
      return aggregate_id::D_SUM;
    } else if (str == "MIN") {
      return aggregate_id::D_MIN;
    } else if (str == "MAX") {
      return aggregate_id::D_MAX;
    } else if (str == "CNT") {
      return aggregate_id::D_CNT;
    } else {
      THROW(invalid_operation_exception, "No such aggregate type.");
    }
  }
};

using aggregate_fn = numeric (*)(const numeric& v1, const numeric& v2);
using zero_fn = numeric (*)(const data_type& type);

struct aggregator {
  aggregate_fn agg;
  zero_fn zero;
};

// Standard aggregates: sum, min, max, count
inline numeric sum_agg(const numeric& a, const numeric& b) {
  return a + b;
}

inline numeric min_agg(const numeric& a, const numeric& b) {
  return a < b ? a : b;
}

inline numeric max_agg(const numeric& a, const numeric& b) {
  return a < b ? b : a;
}

inline numeric count_agg(const numeric& a, const numeric& b) {
  return a + b;
}

inline numeric invalid_agg(const numeric& a, const numeric& b) {
  throw invalid_operation_exception("Invalid aggregation performed.");
}

inline numeric sum_zero(const data_type& type) {
  return numeric(type, type.zero());
}

inline numeric min_zero(const data_type& type) {
  return numeric(type, type.max());
}

inline numeric max_zero(const data_type& type) {
  return numeric(type, type.min());
}

inline numeric count_zero(const data_type& type) {
  return numeric(type, type.zero());
}

inline numeric invalid_zero(const data_type& type) {
  throw invalid_operation_exception("Invalid zero op.");
}

static aggregator sum_aggregator = { sum_agg, sum_zero };
static aggregator min_aggregator = { min_agg, min_zero };
static aggregator max_aggregator = { max_agg, max_zero };
static aggregator count_aggregator = { count_agg, count_zero };
static aggregator invalid_aggregator = { invalid_agg, invalid_zero };

class aggregate;

static std::vector<aggregator> aggregators { sum_aggregator, min_aggregator,
    max_aggregator, count_aggregator };

struct aggregate_node {
  aggregate_node(numeric agg, uint64_t version, aggregate_node *next)
      : value_(agg),
        version_(version),
        next_(next) {
  }

  inline numeric value() const {
    return value_;
  }

  inline uint64_t version() const {
    return version_;
  }

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
   * Default constructor
   */
  aggregate_list()
      : head_(nullptr),
        agg_(invalid_aggregator),
        type_(NONE_TYPE) {

  }

  /**
   * Constructor for aggregate
   *
   * @param agg Aggregate function pointer.
   */
  aggregate_list(data_type type, aggregator agg)
      : head_(nullptr),
        agg_(agg),
        type_(type) {
  }

  /**
   * Constructor for aggregate
   *
   * @param id Aggregate ID.
   */
  aggregate_list(data_type type, aggregate_id id)
      : head_(nullptr),
        agg_(aggregators[id]),
        type_(type) {
  }

  /**
   * Default destructor.
   */
  ~aggregate_list() = default;

  void init(data_type type, aggregator agg) {
    type_ = type;
    agg_ = agg;
  }

  void init(data_type type, aggregate_id id) {
    type_ = type;
    agg_ = aggregators[id];
  }

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
  aggregate()
      : type_(NONE_TYPE),
        agg_(invalid_aggregator),
        aggs_(nullptr) {
  }

  aggregate(const data_type& type, aggregate_id id)
      : type_(type),
        agg_(aggregators[id]),
        aggs_(new aggregate_list[thread_manager::get_max_concurrency()]) {
    for (int i = 0; i < thread_manager::get_max_concurrency(); i++)
      aggs_[i].init(type, id);
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

#endif /* DIALOG_AGGREGATE_H_ */

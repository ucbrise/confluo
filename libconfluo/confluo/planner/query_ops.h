#ifndef CONFLUO_PLANNER_QUERY_OPS_H_
#define CONFLUO_PLANNER_QUERY_OPS_H_

#include "container/data_log.h"
#include "container/lazy/stream.h"
#include "container/radix_tree.h"
#include "container/record_offset_range.h"
#include "schema/schema.h"
#include "container/cursor/offset_cursors.h"

namespace confluo {
namespace planner {

/**
 * Type of operation that is executed
 */
enum query_op_type {
  D_NO_OP = 0,
  D_SCAN_OP = 1,
  D_NO_VALID_INDEX_OP = 2,
  D_INDEX_OP = 3
};

/**
 * @brief Query operation
 */
class query_op {
 public:
     /**
      * query_op
      *
      * @param op The op
      */
  query_op(const query_op_type& op)
      : op_(op) {
  }

  /**
   * ~query_op
   */
  virtual ~query_op() {
  }

  /**
   * op_type
   *
   * @return query_op_type
   */
  query_op_type op_type() {
    return op_;
  }

  /**
   * cost
   *
   * @return uint64_t
   */
  virtual uint64_t cost() const = 0;

  /**
   * to_string
   *
   * @return std::string
   */
  virtual std::string to_string() const = 0;

 private:
  query_op_type op_;
};

/**
 * no operation
 */
class no_op : public query_op {
 public:
     /**
      * no_op
      */
  no_op()
      : query_op(query_op_type::D_NO_OP) {
  }

  /**
   * to_string
   *
   * @return std::string
   */
  virtual std::string to_string() const override {
    return "no_op";
  }

  /**
   * cost
   *
   * @return uint64_t
   */
  virtual uint64_t cost() const override {
    return UINT64_C(0);
  }
};

/**
 * No valid index operation
 */
class no_valid_index_op : public query_op {
 public:
     /**
      * no_valid_index_op
      */
  no_valid_index_op()
      : query_op(query_op_type::D_NO_VALID_INDEX_OP) {
  }

  /**
   * to_string
   *
   * @return std::string
   */
  virtual std::string to_string() const override {
    return "no_valid_index_op";
  }

  /**
   * cost
   *
   * @return uint64_t
   */
  virtual uint64_t cost() const override {
    return UINT64_MAX;
  }
};

/**
 * Full scan operation
 */
class full_scan_op : public query_op {
 public:
  full_scan_op()
      : query_op(query_op_type::D_SCAN_OP) {
  }

  /**
   * to_string
   *
   * @return std::string
   */
  virtual std::string to_string() const override {
    return "full_scan";
  }

  /**
   * cost
   *
   * @return uint64_t
   */
  virtual uint64_t cost() const override {
    return UINT64_MAX;
  }
};

/**
 * Index operation
 */
class index_op : public query_op {
 public:
  typedef std::pair<byte_string, byte_string> key_range;

  index_op(const index::radix_index* index, const key_range& range)
      : query_op(query_op_type::D_INDEX_OP),
        index_(index),
        range_(range) {
  }

  /**
   * to_string
   *
   * @return std::string
   */
  virtual std::string to_string() const override {
    return "range(" + range_.first.to_string() + "," + range_.second.to_string()
        + ")" + " on index=" + index_->to_string();
  }

  /**
   * cost
   *
   * @return uint64_t
   */
  virtual uint64_t cost() const override {
    return index_->approx_count(range_.first, range_.second);
  }

  index::radix_index::rt_result query_index() {
    return index_->range_lookup(range_.first, range_.second);
  }

 private:
  const index::radix_index* index_;
  key_range range_;
};

}
}

#endif /* CONFLUO_PLANNER_QUERY_OPS_H_ */

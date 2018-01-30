#ifndef CONFLUO_PLANNER_QUERY_OPS_H_
#define CONFLUO_PLANNER_QUERY_OPS_H_

#include "container/data_log.h"
#include "container/lazy/stream.h"
#include "container/radix_tree.h"
#include "container/record_offset_range.h"
#include "schema/schema.h"

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
 * A query operation
 */
class query_op {
 public:
  /**
   * Constructs a query operation from a type of query op
   *
   * @param op The type of query operation
   */
  query_op(const query_op_type& op)
      : op_(op) {
  }

  /**
   * Destructs the query operation
   */
  virtual ~query_op() {
  }

  /**
   * Gets the operation type
   *
   * @return The query operation type
   */
  query_op_type op_type() {
    return op_;
  }

  /**
   * Gets the cost of the query operation
   *
   * @return The query operation cost
   */
  virtual uint64_t cost() const = 0;

  /**
   * Gets a string representation of the query operation
   *
   * @return The string representation of the query operation
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
   * Constructs a no op query operation
   */
  no_op()
      : query_op(query_op_type::D_NO_OP) {
  }

  /**
   * Gets a string representation of the no op
   *
   * @return A string containing the representation of no op
   */
  virtual std::string to_string() const override {
    return "no_op";
  }

  /**
   * Gets the cost of a no op
   *
   * @return The cost of a no op
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
   * Constructs a no valid index query operation
   */
  no_valid_index_op()
      : query_op(query_op_type::D_NO_VALID_INDEX_OP) {
  }

  /**
   * Gets a string representation of the no valid index operation
   *
   * @return A string containing the representation of a no valid index
   * operation
   */
  virtual std::string to_string() const override {
    return "no_valid_index_op";
  }

  /**
   * Gets the cost of the no valid index operation
   *
   * @return The cost of the no valid index operation
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
  /**
   * Constructs a full scan query operation
   *
   * @param dlog The data log to perform the operation on
   * @param schema The schema of the data log
   * @param expr The expression of the operation
   */
  full_scan_op(const data_log* dlog, const schema_t* schema,
               const compiled_expression& expr)
      : query_op(query_op_type::D_SCAN_OP),
        dlog_(dlog),
        schema_(schema),
        expr_(expr) {
  }

  /**
   * Gets a string representation of the full scan operation
   *
   * @return A formatted string containing the name and expression
   * of the operation
   */
  virtual std::string to_string() const override {
    return "full_scan(" + expr_.to_string() + ")";
  }

  /**
   * Gets the cost of the full scan operation
   *
   * @return The cost of the full scan operation
   */
  virtual uint64_t cost() const override {
    return UINT64_MAX;
  }

  /**
   * Executes the full scan operation on the data log
   *
   * @param version The version to perform the operation on
   *
   * @return A stream containing the result of the operation
   */
  lazy::stream<record_t> execute(uint64_t version) const {
    const data_log* d = dlog_;
    const schema_t* s = schema_;
    static auto to_record = [d, s](uint64_t offset) -> record_t {
      ro_data_ptr ptr;
      d->cptr(offset, ptr);
      return s->apply(offset, ptr);
    };
    auto expr_check = [this](const record_t& r) -> bool {
      return this->expr_.test(r);
    };
    auto r = record_offset_range(version, schema_->record_size());
    return lazy::container_to_stream(r).map(to_record).filter(expr_check);
  }

  // TODO: Add tests
  // TODO: Clean up
  numeric aggregate(uint64_t version, uint16_t field_idx,
                    aggregate_type agg) const {
    auto r = record_offset_range(version, schema_->record_size());
    switch (agg) {
      case aggregate_type::D_CNT: {
        int64_t count = 0;
        for (auto o : r) {
          if (o < version) {
            ro_data_ptr ptr;
            dlog_->cptr(o, ptr);
            if (expr_.test(schema_->apply(o, ptr))) {
              count++;
            }
          }
        }
        return numeric(count);
      }
      case aggregate_type::D_MIN: {
        numeric min((*schema_)[field_idx].max());
        for (auto o : r) {
          if (o < version) {
            ro_data_ptr ptr;
            dlog_->cptr(o, ptr);
            auto rec = schema_->apply(o, ptr);
            if (expr_.test(rec)) {
              numeric val(rec[field_idx].value());
              min = (val < min) ? val : min;
            }
          }
        }
        return min;
      }
      case aggregate_type::D_MAX: {
        numeric max((*schema_)[field_idx].min());
        for (auto o : r) {
          if (o < version) {
            ro_data_ptr ptr;
            dlog_->cptr(o, ptr);
            auto rec = schema_->apply(o, ptr);
            if (expr_.test(rec)) {
              numeric val(rec[field_idx].value());
              max = (val > max) ? val : max;
            }
          }
        }
        return max;
      }
      case aggregate_type::D_SUM: {
        numeric sum((*schema_)[field_idx].type());
        for (auto o : r) {
          if (o < version) {
            ro_data_ptr ptr;
            dlog_->cptr(o, ptr);
            auto rec = schema_->apply(o, ptr);
            if (expr_.test(rec)) {
              sum = sum + numeric(rec[field_idx].value());
            }
          }
        }
        return sum;
      }
      default: {
        THROW(invalid_operation_exception, "Unknown aggregate type");
      }
    }
  }

 private:
  const data_log* dlog_;
  const schema_t* schema_;
  const compiled_expression& expr_;
};

/**
 * Index operation
 */
class index_op : public query_op {
 public:
  typedef std::pair<byte_string, byte_string> key_range;

  /**
   * Constructs an index query operation
   *
   * @param dlog The data log to perform the operation on
   * @param index The index used
   * @param schema The schema of the data log
   * @param range The range of keys to work over
   * @param m The compiled minterm
   */
  index_op(const data_log* dlog, const index::radix_index* index,
           const schema_t* schema, const key_range& range,
           const compiled_minterm& m)
      : query_op(query_op_type::D_INDEX_OP),
        dlog_(dlog),
        index_(index),
        schema_(schema),
        range_(range) {
    expr_.insert(m);
  }

  /**
   * Gets a string representation of the index operation
   *
   * @return Information about the index operation in a string
   */
  virtual std::string to_string() const override {
    return "range(" + range_.first.to_string() + "," + range_.second.to_string()
        + ")" + " on index=" + index_->to_string() + " + filter("
        + expr_.to_string() + ")";
  }

  /**
   * Gets the cost of the index operation
   *
   * @return The cost of the index operation
   */
  virtual uint64_t cost() const override {
    return index_->approx_count(range_.first, range_.second);
  }

  /**
   * Executes the index operation
   *
   * @param version The version to perform the operation on
   *
   * @return A straem of records containing the result of the operation
   */
  lazy::stream<record_t> execute(uint64_t version) const {
    const data_log* d = dlog_;
    const schema_t* s = schema_;
    compiled_expression e = expr_;

    auto version_check = [version](uint64_t offset) -> bool {
      return offset < version;
    };

    auto to_record = [d, s](uint64_t offset) -> record_t {
      ro_data_ptr ptr;
      d->cptr(offset, ptr);
      return s->apply(offset, ptr);
    };

    auto expr_check = [e](const record_t& r) -> bool {
      return e.test(r);
    };

    auto r = index_->range_lookup(range_.first, range_.second);
    return lazy::container_to_stream(r).filter(version_check).map(to_record)
        .filter(expr_check);
  }

  // TODO: Add tests
  // TODO: Clean up
  numeric aggregate(uint64_t version, uint16_t field_idx,
                    aggregate_type agg) const {
    auto r = index_->range_lookup(range_.first, range_.second);
    switch (agg) {
      case aggregate_type::D_CNT: {
        int64_t count = 0;
        for (auto o : r) {
          if (o < version) {
            ro_data_ptr ptr;
            dlog_->cptr(o, ptr);
            if (expr_.test(schema_->apply(o, ptr))) {
              count++;
            }
          }
        }
        return numeric(count);
      }
      case aggregate_type::D_MIN: {
        numeric min((*schema_)[field_idx].max());
        for (auto o : r) {
          if (o < version) {
            ro_data_ptr ptr;
            dlog_->cptr(o, ptr);
            auto rec = schema_->apply(o, ptr);
            if (expr_.test(rec)) {
              numeric val(rec[field_idx].value());
              min = (val < min) ? val : min;
            }
          }
        }
        return min;
      }
      case aggregate_type::D_MAX: {
        numeric max((*schema_)[field_idx].min());
        for (auto o : r) {
          if (o < version) {
            ro_data_ptr ptr;
            dlog_->cptr(o, ptr);
            auto rec = schema_->apply(o, ptr);
            if (expr_.test(rec)) {
              numeric val(rec[field_idx].value());
              max = (val > max) ? val : max;
            }
          }
        }
        return max;
      }
      case aggregate_type::D_SUM: {
        numeric sum((*schema_)[field_idx].type());
        for (auto o : r) {
          if (o < version) {
            ro_data_ptr ptr;
            dlog_->cptr(o, ptr);
            auto rec = schema_->apply(o, ptr);
            if (expr_.test(rec)) {
              sum = sum + numeric(rec[field_idx].value());
            }
          }
        }
        return sum;
      }
      default: {
        THROW(invalid_operation_exception, "Unknown aggregate type");
      }
    }
  }

 private:
  const data_log* dlog_;
  const index::radix_index* index_;
  const schema_t* schema_;
  key_range range_;
  compiled_expression expr_;
};

}
}

#endif /* CONFLUO_PLANNER_QUERY_OPS_H_ */

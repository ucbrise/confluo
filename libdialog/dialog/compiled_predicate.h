#ifndef DIALOG_COMPILED_PREDICATE_H_
#define DIALOG_COMPILED_PREDICATE_H_

#include <string>

#include "record.h"
#include "column.h"
#include "expression.h"
#include "schema.h"
#include "immutable_value.h"
#include "relational_ops.h"
#include "index_filter.h"

namespace dialog {

struct compiled_predicate {
  compiled_predicate(const predicate_t& p, const schema_t& s)
      : op_(p.op) {
    field_name_ = s[p.attr].name();
    field_idx_ = s[p.attr].idx();
    is_field_indexed_ = s[p.attr].is_indexed();
    field_index_id_ = s[p.attr].index_id();
    field_type_ = s[p.attr].type();
    field_index_bucket_size_ = s[p.attr].index_bucket_size();

    try {
      val_ = mutable_value::parse(p.value, field_type_);
      if (field_type_.id == type_id::D_STRING) {
        fprintf(stderr, "compiled predicate value = [%s]\n", val_.to_string().c_str());
      }
    } catch (std::exception& e) {
      THROW(
          parse_exception,
          "Could not parse attribute " + p.attr + " value " + p.value
              + " to type " + field_type_.to_string());
    }

    switch (op_) {
      case relop_id::EQ: {
        rbegin_ = val_.to_key(field_index_bucket_size_);
        rend_ = val_.to_key(field_index_bucket_size_);
//        fprintf(stderr, "EQ Range: (%s, %s)\n", rbegin_.to_string().c_str(),
//                rend_.to_string().c_str());
        break;
      }
      case relop_id::GE: {
        rbegin_ = val_.to_key(field_index_bucket_size_);
        rend_ = s[p.attr].max().to_key(field_index_bucket_size_);
//        fprintf(stderr, "GE Range: (%s, %s)\n", rbegin_.to_string().c_str(),
//                rend_.to_string().c_str());
        break;
      }
      case relop_id::LE: {
        rbegin_ = s[p.attr].min().to_key(field_index_bucket_size_);
        rend_ = val_.to_key(field_index_bucket_size_);
//        fprintf(stderr, "LE Range: (%s, %s)\n", rbegin_.to_string().c_str(),
//                rend_.to_string().c_str());
        break;
      }
      case relop_id::GT: {
        rbegin_ = ++(val_.to_key(field_index_bucket_size_));
        rend_ = s[p.attr].max().to_key(field_index_bucket_size_);
//        fprintf(stderr, "GT Range: (%s, %s)\n", rbegin_.to_string().c_str(),
//                rend_.to_string().c_str());
        break;
      }
      case relop_id::LT: {
        rbegin_ = s[p.attr].min().to_key(field_index_bucket_size_);
        rend_ = --(val_.to_key(field_index_bucket_size_));
//        fprintf(stderr, "LT Range: (%s, %s)\n", rbegin_.to_string().c_str(),
//                rend_.to_string().c_str());
        break;
      }
      default: {
      }
    }
  }

  inline data_type field_type() const {
    return field_type_;
  }

  inline std::string field_name() const {
    return field_name_;
  }

  inline uint32_t field_idx() const {
    return field_idx_;
  }

  inline relop_id op() const {
    return op_;
  }

  inline immutable_value value() const {
    return val_;
  }

  inline bool is_indexed() const {
    return is_field_indexed_;
  }

  inline std::vector<index_filter> idx_filters() const {
    uint32_t id = field_index_id_;
    switch (op_) {
      case relop_id::EQ:
      case relop_id::GE:
      case relop_id::LE:
      case relop_id::GT:
      case relop_id::LT:
        return {index_filter(id, rbegin_.copy(), rend_.copy())};
      default:
        return {};
    }
  }

  inline bool test(const record_t& r) const {
    return immutable_value::relop(op_, r[field_idx_].value(), val_);
  }

  inline bool test(const schema_snapshot& snap, void* data) const {
    return immutable_value::relop(op_, snap.get(data, field_idx_), val_);
  }

  inline std::string to_string() const {
    return field_name_ + relop_utils::op_to_str(op_) + val_.to_string();
  }

  inline bool operator<(const compiled_predicate& other) const {
    return to_string() < other.to_string();
  }

 private:
  std::string field_name_;
  uint32_t field_idx_;
  bool is_field_indexed_;
  uint32_t field_index_id_;
  double field_index_bucket_size_;
  data_type field_type_;

  relop_id op_;
  mutable_value val_;

  byte_string rbegin_;
  byte_string rend_;
};

}

#endif /* DIALOG_COMPILED_PREDICATE_H_ */

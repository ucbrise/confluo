#ifndef PARSER_EXPRESSION_COMPILER_H_
#define PARSER_EXPRESSION_COMPILER_H_

#include "expression_parser.h"
#include "index_filter.h"
#include "schema.h"
#include "schema_snapshot.h"

namespace dialog {
namespace parser {

namespace spirit = boost::spirit;

struct compiled_predicate {
  compiled_predicate(const std::string& attr, int op, const std::string& value,
                     const schema_t& s)
      : op_(static_cast<relop_id>(op)) {
    field_name_ = s[attr].name();
    field_idx_ = s[attr].idx();
    is_field_indexed_ = s[attr].is_indexed();
    field_index_id_ = s[attr].index_id();
    field_type_ = s[attr].type();
    field_index_bucket_size_ = s[attr].index_bucket_size();

    try {
      val_ = mutable_value::parse(value, field_type_);
    } catch (std::exception& e) {
      THROW(
          parse_exception,
          "Could not parse attribute " + attr + " value " + value + " to type "
              + field_type_.to_string());
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
        rend_ = s[attr].max().to_key(field_index_bucket_size_);
//        fprintf(stderr, "GE Range: (%s, %s)\n", rbegin_.to_string().c_str(),
//                rend_.to_string().c_str());
        break;
      }
      case relop_id::LE: {
        rbegin_ = s[attr].min().to_key(field_index_bucket_size_);
        rend_ = val_.to_key(field_index_bucket_size_);
//        fprintf(stderr, "LE Range: (%s, %s)\n", rbegin_.to_string().c_str(),
//                rend_.to_string().c_str());
        break;
      }
      case relop_id::GT: {
        rbegin_ = ++(val_.to_key(field_index_bucket_size_));
        rend_ = s[attr].max().to_key(field_index_bucket_size_);
//        fprintf(stderr, "GT Range: (%s, %s)\n", rbegin_.to_string().c_str(),
//                rend_.to_string().c_str());
        break;
      }
      case relop_id::LT: {
        rbegin_ = s[attr].min().to_key(field_index_bucket_size_);
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

struct compiled_minterm {
  typedef std::set<compiled_predicate>::iterator iterator;
  typedef std::set<compiled_predicate>::const_iterator const_iterator;

  compiled_minterm() = default;

  compiled_minterm(const compiled_minterm& other)
      : m_(other.m_) {
  }

  inline void add(const compiled_predicate& p) {
    m_.insert(p);
  }

  inline void add(compiled_predicate&& p) {
    m_.insert(std::move(p));
  }

  inline bool test(const record_t& r) const {
    for (auto& p : m_)
      if (!p.test(r))
        return false;
    return true;
  }

  inline bool test(const schema_snapshot& snap, void* data) const {
    for (auto& p : m_)
      if (!p.test(snap, data))
        return false;
    return true;
  }

  std::string to_string() const {
    std::string s = "";
    size_t size = m_.size();
    size_t i = 0;
    for (auto& p : m_) {
      s += p.to_string();
      if (++i < size)
        s += " and ";
    }
    return s;
  }

  iterator begin() {
    return m_.begin();
  }

  iterator end() {
    return m_.end();
  }

  const_iterator begin() const {
    return m_.begin();
  }

  const_iterator end() const {
    return m_.end();
  }

  inline bool operator<(const compiled_minterm& other) const {
    return to_string() < other.to_string();
  }

  size_t size() const {
    return m_.size();
  }

 private:
  std::set<compiled_predicate> m_;
};

struct compiled_expression : public std::set<compiled_minterm> {
  inline bool test(const record_t& r) const {
    if (empty())
      return true;

    for (auto& p : *this)
      if (p.test(r))
        return true;

    return false;
  }

  inline bool test(const schema_snapshot& snap, void* data) const {
      if (empty())
        return true;

      for (auto& p : *this)
        if (p.test(snap, data))
          return true;

      return false;
    }

  std::string to_string() const {
    std::string ret = "";
    size_t s = size();
    size_t i = 0;
    for (auto& p : *this) {
      ret += p.to_string();
      if (++i < s - 1)
        ret += " or ";
    }
    return ret;
  }
};

class utree_expand_conjunction {
 public:
  typedef compiled_expression result_type;

  utree_expand_conjunction(const compiled_minterm& m, const schema_t& schema)
      : m_(m),
        schema_(schema) {
  }

  template<typename T>
  result_type operator()(T) const {
    throw parse_exception(std::string("Unrecognized type ") + typeid(T).name());
  }

  result_type operator()(spirit::function_base const&) const {
    throw parse_exception("Functions not supported");
  }

  template<typename Iterator>
  result_type operator()(boost::iterator_range<Iterator> const& range) const {
    typedef typename boost::iterator_range<Iterator>::const_iterator iterator;
    result_type e;
    iterator i = range.begin();
    int op = spirit::utree::visit(*i, utree_to_op());
    switch (op) {
      case relop_id::EQ:
      case relop_id::NEQ:
      case relop_id::LT:
      case relop_id::GT:
      case relop_id::LE:
      case relop_id::GE: {
        compiled_minterm right = m_;
        std::string attr = spirit::utree::visit(*(++i), utree_to_string());
        std::string value = spirit::utree::visit(*(++i), utree_to_string());
        right.add(compiled_predicate(attr, op, value, schema_));
        e.insert(right);
        break;
      }
      case and_or::OR: {
        compiled_expression left = spirit::utree::visit(*(++i), *this);
        compiled_expression right = spirit::utree::visit(*(++i), *this);
        std::set_union(left.begin(), left.end(), right.begin(), right.end(),
                       std::inserter(e, e.end()));
        break;
      }
      case and_or::AND: {
        compiled_expression lor = spirit::utree::visit(*(++i), *this);
        auto r = *(++i);
        for (auto& lor_m : lor) {
          result_type tmp = spirit::utree::visit(
              r, utree_expand_conjunction(lor_m, schema_));
          std::set_union(e.begin(), e.end(), tmp.begin(), tmp.end(),
                         std::inserter(e, e.end()));
        }
        break;
      }
      default: {
        throw parse_exception("Unexpected op:" + std::to_string(op));
      }
    }
    return e;
  }

 private:
  const compiled_minterm& m_;
  const schema_t& schema_;
};

class utree_compile_expression {
 public:
  typedef compiled_expression result_type;

  utree_compile_expression(const schema_t& schema)
      : schema_(schema) {
  }

  template<typename T>
  result_type operator()(T) const {
    throw parse_exception(std::string("Unrecognized type ") + typeid(T).name());
  }

  result_type operator()(spirit::function_base const&) const {
    throw parse_exception("Functions not supported");
  }

  template<typename Iterator>
  result_type operator()(boost::iterator_range<Iterator> const& range) const {
    typedef typename boost::iterator_range<Iterator>::const_iterator iterator;
    result_type e;
    iterator i = range.begin();
    int op = spirit::utree::visit(*i, utree_to_op());
    switch (op) {
      case relop_id::EQ:
      case relop_id::NEQ:
      case relop_id::LT:
      case relop_id::GT:
      case relop_id::LE:
      case relop_id::GE: {
        compiled_minterm m;
        std::string attr = spirit::utree::visit(*(++i), utree_to_string());
        std::string value = spirit::utree::visit(*(++i), utree_to_string());
        m.add(compiled_predicate(attr, op, value, schema_));
        e.insert(m);
        break;
      }
      case and_or::OR: {
        result_type left = spirit::utree::visit(*(++i), *this);
        result_type right = spirit::utree::visit(*(++i), *this);
        std::set_union(left.begin(), left.end(), right.begin(), right.end(),
                       std::inserter(e, e.end()));
        break;
      }
      case and_or::AND: {
        result_type left = spirit::utree::visit(*(++i), *this);
        auto r = *(++i);
        for (auto& m : left) {
          result_type tmp = spirit::utree::visit(
              r, utree_expand_conjunction(m, schema_));
          std::set_union(e.begin(), e.end(), tmp.begin(), tmp.end(),
                         std::inserter(e, e.end()));
        }
        break;
      }
      default: {
        throw parse_exception("Unexpected op:" + std::to_string(op));
      }
    }
    return e;
  }

 private:
  const schema_t& schema_;
};

static compiled_expression compile_expression(const spirit::utree& e,
                                              const schema_t& schema) {
  return spirit::utree::visit(e, utree_compile_expression(schema));
}

}
}

#endif /* PARSER_EXPRESSION_COMPILER_H_ */

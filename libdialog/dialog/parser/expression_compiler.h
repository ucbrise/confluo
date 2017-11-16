#ifndef PARSER_EXPRESSION_COMPILER_H_
#define PARSER_EXPRESSION_COMPILER_H_

#include "expression_parser.h"
#include "schema.h"
#include "schema_snapshot.h"

namespace confluo {
namespace parser {

namespace spirit = boost::spirit;

struct compiled_predicate {
  compiled_predicate(const std::string& attr, int op, const std::string& value,
                     const schema_t& s)
      : field_name_(s[attr].name()),
        field_idx_(s[attr].idx()),
        op_(static_cast<reational_op_id>(op)),
        val_(mutable_value::parse(value, s[attr].type())) {
  }

  inline std::string const& field_name() const {
    return field_name_;
  }

  inline uint32_t field_idx() const {
    return field_idx_;
  }

  inline reational_op_id op() const {
    return op_;
  }

  inline immutable_value const& value() const {
    return val_;
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
  reational_op_id op_;
  mutable_value val_;
};

struct compiled_minterm : public std::set<compiled_predicate> {
  inline void add(const compiled_predicate& p) {
    insert(p);
  }

  inline void add(compiled_predicate&& p) {
    insert(std::move(p));
  }

  inline bool test(const record_t& r) const {
    for (auto& p : *this)
      if (!p.test(r))
        return false;
    return true;
  }

  inline bool test(const schema_snapshot& snap, void* data) const {
    for (auto& p : *this)
      if (!p.test(snap, data))
        return false;
    return true;
  }

  std::string to_string() const {
    std::string s = "";
    size_t i = 0;
    for (auto& p : *this) {
      s += p.to_string();
      if (++i < size())
        s += " and ";
    }
    return s;
  }

  inline bool operator<(const compiled_minterm& other) const {
    return to_string() < other.to_string();
  }
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
      case reational_op_id::EQ:
      case reational_op_id::NEQ:
      case reational_op_id::LT:
      case reational_op_id::GT:
      case reational_op_id::LE:
      case reational_op_id::GE: {
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
      case reational_op_id::EQ:
      case reational_op_id::NEQ:
      case reational_op_id::LT:
      case reational_op_id::GT:
      case reational_op_id::LE:
      case reational_op_id::GE: {
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

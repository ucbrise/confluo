#include "parser/expression_compiler.h"

namespace confluo {
namespace parser {

compiled_predicate::compiled_predicate(const std::string &attr, int op, const std::string &value, const schema_t &s)
    : field_name_(s[attr].name()),
      field_idx_(s[attr].idx()),
      op_(static_cast<reational_op_id>(op)),
      val_(mutable_value::parse(value, s[attr].type())) {
}

std::string const &compiled_predicate::field_name() const {
  return field_name_;
}

uint32_t compiled_predicate::field_idx() const {
  return field_idx_;
}

reational_op_id compiled_predicate::op() const {
  return op_;
}

immutable_value const &compiled_predicate::value() const {
  return val_;
}

bool compiled_predicate::test(const record_t &r) const {
  return immutable_value::relop(op_, r[field_idx_].value(), val_);
}

bool compiled_predicate::test(const schema_snapshot &snap, void *data) const {
  return immutable_value::relop(op_, snap.get(data, field_idx_), val_);
}

std::string compiled_predicate::to_string() const {
  return field_name_ + relop_utils::op_to_str(op_) + val_.to_string();
}

bool compiled_predicate::operator<(const compiled_predicate &other) const {
  return to_string() < other.to_string();
}

void compiled_minterm::add(const compiled_predicate &p) {
  insert(p);
}

void compiled_minterm::add(compiled_predicate &&p) {
  insert(std::move(p));
}

bool compiled_minterm::test(const record_t &r) const {
  for (auto& p : *this)
    if (!p.test(r))
      return false;
  return true;
}

bool compiled_minterm::test(const schema_snapshot &snap, void *data) const {
  for (auto& p : *this)
    if (!p.test(snap, data))
      return false;
  return true;
}

std::string compiled_minterm::to_string() const {
  std::string s = "";
  size_t i = 0;
  for (auto& p : *this) {
    s += p.to_string();
    if (++i < size())
      s += " and ";
  }
  return s;
}

bool compiled_minterm::operator<(const compiled_minterm &other) const {
  return to_string() < other.to_string();
}

bool compiled_expression::test(const record_t &r) const {
  if (empty())
    return true;

  for (auto& p : *this)
    if (p.test(r))
      return true;

  return false;
}

bool compiled_expression::test(const schema_snapshot &snap, void *data) const {
  if (empty())
    return true;

  for (auto& p : *this)
    if (p.test(snap, data))
      return true;

  return false;
}

std::string compiled_expression::to_string() const {
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

utree_expand_conjunction::utree_expand_conjunction(const compiled_minterm &m, const schema_t &schema)
    : m_(m),
      schema_(schema) {
}

utree_expand_conjunction::result_type utree_expand_conjunction::operator()(spirit::function_base const &) const {
  throw parse_exception("Functions not supported");
}

utree_compile_expression::utree_compile_expression(const schema_t &schema)
    : schema_(schema) {
}

utree_compile_expression::result_type utree_compile_expression::operator()(spirit::function_base const &) const {
  throw parse_exception("Functions not supported");
}

compiled_expression compile_expression(const spirit::utree &e, const schema_t &schema) {
  return spirit::utree::visit(e, utree_compile_expression(schema));
}
}
}
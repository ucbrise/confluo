#include "parser/expression_parser.h"

namespace confluo {
namespace parser {

void utree_dbg_print::operator()(spirit::utf8_string_range_type const &str) const {
  typedef spirit::utf8_string_range_type::const_iterator iterator;
  iterator i = str.begin();
  fprintf(stderr, " \"");
  for (; i != str.end(); ++i)
    fprintf(stderr, "%c", *i);
  fprintf(stderr, "\" ");
}

void utree_dbg_print::operator()(int op) const {
  if (op < 6) {
    reational_op_id id = static_cast<reational_op_id>(op);
    fprintf(stderr, " %s ", relop_utils::op_to_str(id).c_str());
  } else {
    fprintf(stderr, " %s ", (op == and_or::AND) ? "AND" : "OR");
  }
}

void utree_dbg_print::operator()(spirit::function_base const &) const {
  return (*this)("<function>\n");
}

utree_to_op::result_type utree_to_op::operator()(spirit::function_base const &) const {
  throw parse_exception("Functions not supported");
}

utree_to_op::result_type utree_to_op::operator()(int op) const {
  return op;
}

utree_to_string::result_type utree_to_string::operator()(spirit::function_base const &) const {
  throw parse_exception("Functions not supported");
}

utree_to_string::result_type utree_to_string::operator()(spirit::utf8_string_range_type const &str) const {
  typedef spirit::utf8_string_range_type::const_iterator iterator;
  iterator it = str.begin(), end = str.end();

  result_type out;
  for (; it != end; ++it)
    out += *it;

  return out;
}

spirit::utree utree_negate::operator()(spirit::utf8_string_range_type const &str) const {
  return str;
}

spirit::utree utree_negate::operator()(int op) const {
  switch (op) {
    case reational_op_id::EQ:return spirit::utree(reational_op_id::NEQ);
    case reational_op_id::NEQ:return spirit::utree(reational_op_id::EQ);
    case reational_op_id::LT:return spirit::utree(reational_op_id::GE);
    case reational_op_id::GT:return spirit::utree(reational_op_id::LE);
    case reational_op_id::LE:return spirit::utree(reational_op_id::GT);
    case reational_op_id::GE:return spirit::utree(reational_op_id::LT);
    case and_or::AND:return spirit::utree(and_or::OR);
    case and_or::OR:return spirit::utree(and_or::AND);
  }
  return spirit::utree::invalid_type();
}

spirit::utree utree_negate::operator()(spirit::function_base const &) const {
  throw parse_exception("Function is not supported yet");
}

expr::expr(int op)
    : op(op) {
}

void expr::operator()(spirit::utree &t, spirit::utree const &rhs) const {
  spirit::utree lhs;
  lhs.swap(t);
  t.push_back(op);
  t.push_back(lhs);
  t.push_back(rhs);
}

pred::pred(const int op)
    : op(op) {
}

void pred::operator()(spirit::utree &t, spirit::utree const &rhs) const {
  spirit::utree lhs;
  lhs.swap(t);
  t.push_back(op);
  t.push_back(lhs);
  t.push_back(rhs);
}

void negate_expr::operator()(spirit::utree &expr, spirit::utree const &rhs) const {
  expr.clear();
  expr = spirit::utree::visit(rhs, utree_negate());
}

spirit::utree parse_expression(const std::string &e) {
  using boost::spirit::utree;
  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef expression_parser<iterator_type> grammar;
  grammar g;
  std::string::const_iterator iter = e.begin();
  std::string::const_iterator end = e.end();
  utree ut;
  bool r = phrase_parse(iter, end, g, space, ut);
  if (iter != end || !r) {
    std::string rest(iter, end);
    throw parse_exception(std::string("Parse failed at ") + rest);
  }
  return ut;
}
}
}
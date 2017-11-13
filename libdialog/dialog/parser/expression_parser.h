#ifndef DIALOG_EXPRESSION_PARSER_H_
#define DIALOG_EXPRESSION_PARSER_H_

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_function.hpp>

#include <string>

#include "../types/relational_ops.h"

namespace dialog {
namespace parser {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace spirit = boost::spirit;

enum and_or
  : int {
    AND = 6,
  OR = 7
};

class utree_dbg_print {
 public:
  typedef void result_type;

  utree_dbg_print() = default;

  template<typename T>
  void operator()(T) const {
    fprintf(stderr, " <!!!Unrecognized type %s!!!> ", typeid(T).name());
  }

  template<typename Iterator>
  void operator()(boost::iterator_range<Iterator> const& range) const {
    typedef typename boost::iterator_range<Iterator>::const_iterator iterator;
    fprintf(stderr, "(");
    for (iterator i = range.begin(); i != range.end(); ++i) {
      spirit::utree::visit(*i, *this);
    }
    fprintf(stderr, ")");
  }

  void operator()(spirit::utf8_string_range_type const& str) const {
    typedef spirit::utf8_string_range_type::const_iterator iterator;
    iterator i = str.begin();
    fprintf(stderr, " \"");
    for (; i != str.end(); ++i)
      fprintf(stderr, "%c", *i);
    fprintf(stderr, "\" ");
  }

  void operator()(int op) const {
    if (op < 6) {
      reational_op_id id = static_cast<reational_op_id>(op);
      fprintf(stderr, " %s ", relop_utils::op_to_str(id).c_str());
    } else {
      fprintf(stderr, " %s ", (op == and_or::AND) ? "AND" : "OR");
    }
  }

  void operator()(spirit::function_base const&) const {
    return (*this)("<function>\n");
  }
};

class utree_to_op {
 public:
  typedef int result_type;

  utree_to_op() = default;

  template<typename T>
  result_type operator()(T) const {
    throw parse_exception(std::string("Unrecognized type ") + typeid(T).name());
  }

  result_type operator()(spirit::function_base const&) const {
    throw parse_exception("Functions not supported");
  }

  result_type operator()(int op) const {
    return op;
  }
};

class utree_to_string {
 public:
  typedef std::string result_type;

  utree_to_string() = default;

  template<typename T>
  result_type operator()(T) const {
    throw parse_exception(std::string("Unrecognized type ") + typeid(T).name());
  }

  result_type operator()(spirit::function_base const&) const {
    throw parse_exception("Functions not supported");
  }

  result_type operator()(spirit::utf8_string_range_type const& str) const {
    typedef spirit::utf8_string_range_type::const_iterator iterator;
    iterator it = str.begin(), end = str.end();

    result_type out;
    for (; it != end; ++it)
      out += *it;

    return out;
  }
};

class utree_negate {
 public:
  typedef spirit::utree result_type;

  utree_negate() = default;

  template<typename T>
  spirit::utree operator()(T) const {
    throw parse_exception(std::string("Unrecognized type ") + typeid(T).name());
  }

  template<typename Iterator>
  spirit::utree operator()(boost::iterator_range<Iterator> const& range) const {
    typedef typename boost::iterator_range<Iterator>::const_iterator iterator;
    spirit::utree ut;
    for (iterator i = range.begin(); i != range.end(); ++i) {
      ut.push_back(spirit::utree::visit(*i, *this));
    }
    return ut;
  }

  spirit::utree operator()(spirit::utf8_string_range_type const& str) const {
    return str;
  }

  spirit::utree operator()(int op) const {
    switch (op) {
      case reational_op_id::EQ:
        return spirit::utree(reational_op_id::NEQ);
      case reational_op_id::NEQ:
        return spirit::utree(reational_op_id::EQ);
      case reational_op_id::LT:
        return spirit::utree(reational_op_id::GE);
      case reational_op_id::GT:
        return spirit::utree(reational_op_id::LE);
      case reational_op_id::LE:
        return spirit::utree(reational_op_id::GT);
      case reational_op_id::GE:
        return spirit::utree(reational_op_id::LT);
      case and_or::AND:
        return spirit::utree(and_or::OR);
      case and_or::OR:
        return spirit::utree(and_or::AND);
    }
    return spirit::utree::invalid_type();
  }

  spirit::utree operator()(spirit::function_base const&) const {
    throw parse_exception("Function is not supported yet");
  }
};

struct expr {
  template<typename T1, typename T2 = void>
  struct result {
    using type = void;
  };

  expr(int op)
      : op(op) {
  }

  void operator()(spirit::utree& t, spirit::utree const& rhs) const {
    spirit::utree lhs;
    lhs.swap(t);
    t.push_back(op);
    t.push_back(lhs);
    t.push_back(rhs);
  }

  int const op;
};

struct pred {
  template<typename T1, typename T2 = void>
  struct result {
    using type = void;
  };

  pred(const int op)
      : op(op) {
  }

  void operator()(spirit::utree& t, spirit::utree const& rhs) const {
    spirit::utree lhs;
    lhs.swap(t);
    t.push_back(op);
    t.push_back(lhs);
    t.push_back(rhs);
  }

  int const op;
};

boost::phoenix::function<pred> const LT = pred(reational_op_id::LT);
boost::phoenix::function<pred> const LE = pred(reational_op_id::LE);
boost::phoenix::function<pred> const GT = pred(reational_op_id::GT);
boost::phoenix::function<pred> const GE = pred(reational_op_id::GE);
boost::phoenix::function<pred> const EQ = pred(reational_op_id::EQ);
boost::phoenix::function<pred> const NEQ = pred(reational_op_id::NEQ);

boost::phoenix::function<expr> const CONJ = expr(and_or::AND);
boost::phoenix::function<expr> const DISJ = expr(and_or::OR);

struct negate_expr {
  template<typename T1, typename T2 = void>
  struct result {
    typedef void type;
  };

  void operator()(spirit::utree& expr, spirit::utree const& rhs) const {
    expr.clear();
    expr = spirit::utree::visit(rhs, utree_negate());
  }
};

boost::phoenix::function<negate_expr> NEG;

/* <expression>::=<term>{"||"<term>}
 * <term>::=<factor>{"&&"<factor>}
 * <factor>::=<predicate> | "!"<factor> | '('<expression>')'
 * <predicate>::= <identifier>"<"<value> | <identifier>">"<value> ...
 * <identifier>::= "alnum*"
 * <value>::= "alnum*"
 */

template<typename I>
struct expression_parser : qi::grammar<I, ascii::space_type, spirit::utree()> {
  expression_parser()
      : expression_parser::base_type(exp) {
    using qi::char_;
    using qi::short_;
    using qi::int_;
    using qi::long_;
    using qi::float_;
    using qi::double_;
    using qi::_val;
    using qi::_1;
    using qi::_a;
    using qi::alpha;
    using qi::alnum;
    using qi::lexeme;
    using qi::raw;
    using qi::omit;
    using qi::no_skip;
    using qi::lit;

    exp = term[_val = _1] >> *("||" >> term[DISJ(_val, _1)]);
    term = factor[_val = _1] >> *("&&" >> factor[CONJ(_val, _1)]);
    factor = predicate[_val = _1] | ("!" >> factor[NEG(_val, _1)])
        | '(' >> exp[_val = _1] >> ')';
    predicate = identifier[_val = _1] >> "<" >> value[LT(_val, _1)]
        | identifier[_val = _1] >> "<=" >> value[LE(_val, _1)]
        | identifier[_val = _1] >> ">" >> value[GT(_val, _1)]
        | identifier[_val = _1] >> ">=" >> value[GE(_val, _1)]
        | identifier[_val = _1] >> "==" >> value[EQ(_val, _1)]
        | identifier[_val = _1] >> "!=" >> value[NEQ(_val, _1)];
    identifier = (alpha | char_("_")) >> *(alnum | char_("_"));
    quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];
    value = +(char_("_+-.") | alnum) | quoted_string;
  }

  qi::rule<I, ascii::space_type, spirit::utree()> exp;
  qi::rule<I, ascii::space_type, spirit::utree()> term;
  qi::rule<I, ascii::space_type, spirit::utree()> factor;
  qi::rule<I, ascii::space_type, spirit::utree()> predicate;
  qi::rule<I, ascii::space_type, std::string()> identifier;
  qi::rule<I, ascii::space_type, std::string()> value;
  qi::rule<I, ascii::space_type, std::string()> quoted_string;
};

static spirit::utree parse_expression(const std::string& e) {
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

#endif /* DIALOG_EXPRESSION_PARSER_H_ */

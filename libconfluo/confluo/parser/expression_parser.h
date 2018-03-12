#ifndef CONFLUO_EXPRESSION_PARSER_H_
#define CONFLUO_EXPRESSION_PARSER_H_

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_function.hpp>

#include <string>
#include <set>

#include "types/relational_ops.h"

namespace confluo {
namespace parser {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace spirit = boost::spirit;

/**
 * Encapsulates and or operators
 */
enum and_or
  : int {
    /** And identifier */
    AND = 6,
  /** Or identifier */
  OR = 7
};

/**
 * Tree that handles printing debug output
 */
class utree_dbg_print {
 public:
  /** The result type of the debug tree */
  typedef void result_type;

  utree_dbg_print() = default;

  /**
   * Handles unrecognized type case
   *
   * @tparam T The data type
   */
  template<typename T>
  void operator()(T) const {
    fprintf(stderr, " <!!!Unrecognized type %s!!!> ", typeid(T).name());
  }

  /**
   * Visits nodes in the range
   *
   * @tparam Iterator that contains the nodes in the range
   * @param range The range of nodes to visit
   */
  template<typename Iterator>
  void operator()(boost::iterator_range<Iterator> const& range) const {
    typedef typename boost::iterator_range<Iterator>::const_iterator iterator;
    fprintf(stderr, "(");
    for (iterator i = range.begin(); i != range.end(); ++i) {
      spirit::utree::visit(*i, *this);
    }
    fprintf(stderr, ")");
  }

  /**
   * Prints the characters in the string
   *
   * @param str The string to print the characters from
   */
  void operator()(spirit::utf8_string_range_type const& str) const {
    typedef spirit::utf8_string_range_type::const_iterator iterator;
    iterator i = str.begin();
    fprintf(stderr, " \"");
    for (; i != str.end(); ++i)
      fprintf(stderr, "%c", *i);
    fprintf(stderr, "\" ");
  }

  /**
   * Prints the corresponding relational operator
   *
   * @param op The identifier of the relational operator
   */
  void operator()(int op) const {
    if (op < 6) {
      reational_op_id id = static_cast<reational_op_id>(op);
      fprintf(stderr, " %s ", relop_utils::op_to_str(id).c_str());
    } else {
      fprintf(stderr, " %s ", (op == and_or::AND) ? "AND" : "OR");
    }
  }

  /**
   * Prints the function
   *
   */
  void operator()(spirit::function_base const&) const {
    return (*this)("<function>\n");
  }
};

/**
 * To operator
 */
class utree_to_op {
 public:
  /** The result type */
  typedef int result_type;

  utree_to_op() = default;

  /**
   * Handles the unrecognized type case for the operator
   * @tparam T The data type
   * @throw parse_exception This type is not recognized
   * @return The result
   */
  template<typename T>
  result_type operator()(T) const {
    throw parse_exception(std::string("Unrecognized type ") + typeid(T).name());
  }

  /**
   * Handles the unrecognized type case for the operator when called
   * on functions
   * @throw parse_exception This type is not recognized
   * @return The result
   */
  result_type operator()(spirit::function_base const&) const {
    throw parse_exception("Functions not supported");
  }

  /**
   * Returns the op id associated
   *
   * @param op The operator identifier
   *
   * @return The result which is the operator identifier
   */
  result_type operator()(int op) const {
    return op;
  }
};

/**
 * To string operation
 */
class utree_to_string {
 public:
  /** The result type */
  typedef std::string result_type;

  utree_to_string() = default;

  /**
   * Handles the case for unrecognized types
   * @tparam T The data type
   * @throw parse_exception The type is not recognized
   * @return The result
   */
  template<typename T>
  result_type operator()(T) const {
    throw parse_exception(std::string("Unrecognized type ") + typeid(T).name());
  }

  /**
   * Handles the case for functions passed in
   * @throw parse_exception Functions are not supported for to string
   * @return The result
   */
  result_type operator()(spirit::function_base const&) const {
    throw parse_exception("Functions not supported");
  }

  /**
   * Iterates through the string and returns the result
   *
   * @param str The string iterator that is iterated through
   *
   * @return The result of combining all of the values in the iterator
   */
  result_type operator()(spirit::utf8_string_range_type const& str) const {
    typedef spirit::utf8_string_range_type::const_iterator iterator;
    iterator it = str.begin(), end = str.end();

    result_type out;
    for (; it != end; ++it)
      out += *it;

    return out;
  }
};

/**
 * Negate operator
 */
class utree_negate {
 public:
  /** The result type */
  typedef spirit::utree result_type;

  utree_negate() = default;

  /**
   * Handles the case for unrecognized types
   * @tparam T The data type
   * @throw parse_exception The type is not recognized
   * @return The tree
   */
  template<typename T>
  spirit::utree operator()(T) const {
    throw parse_exception(std::string("Unrecognized type ") + typeid(T).name());
  }

  /**
   * Visits all of the nodes in the iterator range
   *
   * @tparam Iterator The type of node
   * @param range The range of nodes to visit
   *
   * @return The tree with all of the visited nodes in the range
   */
  template<typename Iterator>
  spirit::utree operator()(boost::iterator_range<Iterator> const& range) const {
    typedef typename boost::iterator_range<Iterator>::const_iterator iterator;
    spirit::utree ut;
    for (iterator i = range.begin(); i != range.end(); ++i) {
      ut.push_back(spirit::utree::visit(*i, *this));
    }
    return ut;
  }

  /**
   * Gets the string contents from a string range
   *
   * @param str The string range to get the contents from
   *
   * @return The tree containing the contents from the specified range
   */
  spirit::utree operator()(spirit::utf8_string_range_type const& str) const {
    return str;
  }

  /**
   * Gets the tree associated with operator id
   *
   * @param op The operator identifier
   *
   * @return The tree associated with the operator identifier
   */
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


  /**
   * Handles the case when functinos are passed in
   * @throw parse_exception Functions are not supported for this operator
   * @return The tree
   */
  spirit::utree operator()(spirit::function_base const&) const {
    throw parse_exception("Function is not supported yet");
  }
};

/**
 * All of the components that make up an expression
 */
struct expr {
  /**
   * The result of an expression
   *
   * @tparam T1 The first type
   * @tparam T2 The second type
   */
  template<typename T1, typename T2 = void>
  struct result {
    /** The type of the result */
    using type = void;
  };

  /**
   * Constructs an expression from an operator id
   *
   * @param op The operator identifier
   */
  expr(int op)
      : op(op) {
  }

  /**
   * Initializes the tree to the components of the expression
   *
   * @param t The tree to initialize
   * @param rhs The right hand side of the expression
   */
  void operator()(spirit::utree& t, spirit::utree const& rhs) const {
    spirit::utree lhs;
    lhs.swap(t);
    t.push_back(op);
    t.push_back(lhs);
    t.push_back(rhs);
  }

  /** The operator in the expression */
  int const op;
};

/**
 * Components that make up a predicate
 */
struct pred {
 /**
  * The result of a predicate
  *
  * @tparam T1 The first type
  * @tparam T2 The second type
  */
  template<typename T1, typename T2 = void>
  struct result {
    /** The type of the result */
    using type = void;
  };

  /**
   * Constructs an empty predicate and initializes the operator
   *
   * @param op The operator id
   */
  pred(const int op)
      : op(op) {
  }

  /**
   * Initializes the tree to the components of the predicate
   *
   * @param t The tree to initialize
   * @param rhs The right hand side of the predicate
   */
  void operator()(spirit::utree& t, spirit::utree const& rhs) const {
    spirit::utree lhs;
    lhs.swap(t);
    t.push_back(op);
    t.push_back(lhs);
    t.push_back(rhs);
  }

  /** The operator associated with the predicate */
  int const op;
};

/** Less than predicate function */
boost::phoenix::function<pred> const LT = pred(reational_op_id::LT);
/** Less than or equal to predicate function */
boost::phoenix::function<pred> const LE = pred(reational_op_id::LE);
/** Greater than predicate function */
boost::phoenix::function<pred> const GT = pred(reational_op_id::GT);
/** Greater than or equal to predicate function */
boost::phoenix::function<pred> const GE = pred(reational_op_id::GE);
/** Equality predicate function */
boost::phoenix::function<pred> const EQ = pred(reational_op_id::EQ);
/** Not equal to predicate function */
boost::phoenix::function<pred> const NEQ = pred(reational_op_id::NEQ);

/** Conjunction predicate function */
boost::phoenix::function<expr> const CONJ = expr(and_or::AND);
/** Disjoint predicate function */
boost::phoenix::function<expr> const DISJ = expr(and_or::OR);

/**
 * Negated expression
 */
struct negate_expr {
  /**
   * The result of the expression
   *
   * @tparam T1 The type of first operand
   * @tparam T2 The type of the second operand
   */
  template<typename T1, typename T2 = void>
  struct result {
    /** The type of the result */
    typedef void type;
  };

  /**
   * Visits the nodes in the expression tree and negate them
   *
   * @param expr The expression to negate
   * @param rhs The right hand side of the expression
   */
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

/**
 * Parser of expressions
 *
 * @tparam I The type of expression
 */
template<typename I>
struct expression_parser : qi::grammar<I, ascii::space_type, spirit::utree()> {
  /**
   * Constructs an expression parser from a grammar
   */
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

  /** Expression rule */
  qi::rule<I, ascii::space_type, spirit::utree()> exp;
  /** Term rule */
  qi::rule<I, ascii::space_type, spirit::utree()> term;
  /** Factor rule */
  qi::rule<I, ascii::space_type, spirit::utree()> factor;
  /** Predicate rule */
  qi::rule<I, ascii::space_type, spirit::utree()> predicate;
  /** Identifier rule */
  qi::rule<I, ascii::space_type, std::string()> identifier;
  /** Value rule */
  qi::rule<I, ascii::space_type, std::string()> value;
  /** Quoted string rule */
  qi::rule<I, ascii::space_type, std::string()> quoted_string;
};

/**
 * Parses the expression from a given string
 *
 * @param e The string to parse the expression from
 *
 * @return The tree representing the contents of the expression
 */
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

#endif /* CONFLUO_EXPRESSION_PARSER_H_ */

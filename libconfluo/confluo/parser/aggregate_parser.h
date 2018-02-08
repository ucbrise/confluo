#ifndef CONFLUO_PARSER_AGGREGATE_PARSER_H_
#define CONFLUO_PARSER_AGGREGATE_PARSER_H_

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_auto.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_function.hpp>

namespace confluo {
namespace parser {

/**
 * Parsed aggregate attributes
 */
struct parsed_aggregate {
  /** The aggregate */
  std::string agg;
  /** The field for which the aggregate is computed over */
  std::string field_name;
};

}
}

BOOST_FUSION_ADAPT_STRUCT(confluo::parser::parsed_aggregate,
                          (std::string, agg) (std::string, field_name))

namespace confluo {
namespace parser {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace spirit = boost::spirit;
namespace phx = boost::phoenix;

/**
 * Aggregate Parser class. Contains operations for parsing aggregates based
 * on grammar rules.
 *
 * @tparam I The type of aggregate
 */
template<typename I>
class aggregate_parser : public qi::grammar<I, ascii::space_type,
    parsed_aggregate()> {
 public:
  /**
   * Initializes rules and grammar for aggregate parser
   */
  aggregate_parser()
      : aggregate_parser::base_type(agg) {
    using qi::alpha;
    using qi::alnum;
    using qi::char_;
    using qi::double_;
    using qi::string;
    using qi::raw;

    agg = agg_type >> '(' >> identifier >> ')';
    agg_type = +alpha;
    identifier = (alpha | char_("_")) >> *(alnum | char_("_"));
  }

  qi::rule<I, ascii::space_type, parsed_aggregate()> agg;
  qi::rule<I, ascii::space_type, std::string()> agg_type;
  qi::rule<I, ascii::space_type, std::string()> identifier;
};

/**
 * Parses the aggregate from a given string
 *
 * @param t The string from which the aggregate is generated from
 *
 * @return The parsed aggregate from the string
 */
parsed_aggregate parse_aggregate(const std::string& t) {
  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef aggregate_parser<iterator_type> grammar;
  grammar g;
  std::string::const_iterator iter = t.begin();
  std::string::const_iterator end = t.end();
  parsed_aggregate pt;
  bool r = phrase_parse(iter, end, g, space, pt);
  if (iter != end || !r) {
    std::string rest(iter, end);
    throw parse_exception(std::string("Parse failed at ") + rest);
  }
  return pt;
}

}
}

#endif /* CONFLUO_PARSER_AGGREGATE_PARSER_H_ */

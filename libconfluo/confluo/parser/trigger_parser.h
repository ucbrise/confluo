#ifndef CONFLUO_PARSER_TRIGGER_PARSER_H_
#define CONFLUO_PARSER_TRIGGER_PARSER_H_

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_auto.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_function.hpp>

namespace confluo {
namespace parser {

/**
 * The attributes that make up a trigger
 */
struct parsed_trigger {
  /** The name of the aggregate */
  std::string aggregate_name;
  /** The name of the relational operator */
  std::string relop;
  /** The threshold event when the trigger occurs */
  std::string threshold;
};

}
}

BOOST_FUSION_ADAPT_STRUCT(
    confluo::parser::parsed_trigger,
    (std::string, aggregate_name)
    (std::string, relop)
    (std::string, threshold))

namespace confluo {
namespace parser {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace spirit = boost::spirit;
namespace phx = boost::phoenix;

/**
 * A trigger parser using grammars and rules
 *
 * @tparam I
 */
template<typename I>
class trigger_parser : public qi::grammar<I, ascii::space_type, parsed_trigger()> {
 public:
  /**
   * Constructs a trigger parser from a grammar
   */
  trigger_parser()
      : trigger_parser::base_type(trig) {
    using qi::alpha;
    using qi::alnum;
    using qi::char_;
    using qi::double_;
    using qi::string;
    using qi::raw;

    trig = identifier >> op >> thresh;
    identifier = (alpha | char_("_")) >> *(alnum | char_("_"));
    op = +char_("<>=!");
    thresh = raw[double_];
  }

  qi::rule<I, ascii::space_type, parsed_trigger()> trig;
  qi::rule<I, ascii::space_type, std::string()> identifier;
  qi::rule<I, ascii::space_type, std::string()> op;
  qi::rule<I, ascii::space_type, std::string()> thresh;
};

/**
 * Parses a trigger from a string
 *
 * @param t The string that the trigger is parsed from
 * @throw parse_exception
 *
 * @return A parsed trigger
 */
parsed_trigger parse_trigger(const std::string& t) {
  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef trigger_parser<iterator_type> grammar;
  grammar g;
  std::string::const_iterator iter = t.begin();
  std::string::const_iterator end = t.end();
  parsed_trigger pt;
  bool r = phrase_parse(iter, end, g, space, pt);
  if (iter != end || !r) {
    std::string rest(iter, end);
    throw parse_exception(std::string("Parse failed at ") + rest);
  }
  return pt;
}

}
}

#endif /* CONFLUO_PARSER_TRIGGER_PARSER_H_ */

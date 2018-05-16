#ifndef CONFLUO_PARSER_SCHEMA_PARSER_H_
#define CONFLUO_PARSER_SCHEMA_PARSER_H_

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <string>

#include "schema/schema.h"

namespace confluo {
namespace parser {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace spirit = boost::spirit;

using kv_pair = std::pair<std::string, std::string>;
using kv_list = std::vector<kv_pair>;

/**
 * Grammar and rules for parsing a schema
 *
 * @tparam I The type of schema parser
 */
template<typename I>
struct schema_parser : public qi::grammar<I, ascii::space_type, kv_list()> {
 public:
  /**
   * Constructs a schema parser
   */
  schema_parser()
      : schema_parser::base_type(sch) {
    using qi::_val;
    using qi::_1;
    using qi::_a;
    using qi::char_;
    using qi::alpha;
    using qi::alnum;
    using qi::lexeme;
    using qi::raw;
    using qi::omit;
    using qi::no_skip;
    using qi::lit;

    sch %= '{' >> +(kv >> ',') >> kv >> '}';
    kv = key >> ':' >> value;
    key = (alpha | char_("_")) >> *(alnum | char_("_"));
    value = +char_("a-zA-Z0-9()");
  }

  /** The schema rule */
  qi::rule<I, ascii::space_type, kv_list()> sch;
  /** The key value rule */
  qi::rule<I, ascii::space_type, kv_pair()> kv;
  /** The key rule */
  qi::rule<I, ascii::space_type, std::string()> key;
  /** The value rule */
  qi::rule<I, ascii::space_type, std::string()> value;
};

/**
 * Parses a schema from a given string
 *
 * @param s The string to parse the schema from
 *
 * @return A vector of columns that contains data that make up the schema
 */
static std::vector<column_t> parse_schema(const std::string& s) {
  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef schema_parser<iterator_type> grammar;
  grammar g;
  std::string::const_iterator iter = s.begin();
  std::string::const_iterator end = s.end();
  kv_list kvs;
  bool r = phrase_parse(iter, end, g, space, kvs);
  if (iter != end || !r) {
    std::string rest(iter, end);
    throw parse_exception(std::string("Parse failed at ") + rest);
  }
  schema_builder b;
  for (const auto& kv : kvs) {
    b.add_column(data_type::from_string(kv.second), kv.first);
  }
  return b.get_columns();
}

}
}

#endif /* CONFLUO_PARSER_SCHEMA_PARSER_H_ */

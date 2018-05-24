#include "parser/aggregate_parser.h"

namespace confluo {
namespace parser {

parsed_aggregate parse_aggregate(const std::string &t) {
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
    throw confluo::parse_exception(std::string("Parse failed at ") + rest);
  }
  return pt;
}

}
}
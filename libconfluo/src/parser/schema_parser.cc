#include "parser/schema_parser.h"

namespace confluo {
namespace parser {

std::vector<column_t> parse_schema(const std::string &s) {
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
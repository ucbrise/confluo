#ifndef CONFLUO_PARSER_TRIGGER_COMPILER_H_
#define CONFLUO_PARSER_TRIGGER_COMPILER_H_

#include "schema/schema.h"
#include "aggregate.h"
#include "trigger_parser.h"
#include "types/numeric.h"
#include "types/relational_ops.h"

namespace confluo {
namespace parser {

struct compiled_trigger {
  aggregate_id agg;
  std::string field_name;
  reational_op_id relop;
  numeric threshold;
};

compiled_trigger compile_trigger(const parsed_trigger& t,
                                 const schema_t& schema) {
  compiled_trigger ct;
  ct.agg = aggop_utils::string_to_agg(t.agg);
  ct.field_name = t.field_name;
  ct.relop = relop_utils::str_to_op(t.relop);
  ct.threshold = numeric::parse(
      t.threshold,
      ct.agg == aggregate_id::D_CNT ? LONG_TYPE : schema[ct.field_name].type());
  return ct;
}

}
}

#endif /* CONFLUO_PARSER_TRIGGER_COMPILER_H_ */

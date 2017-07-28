#ifndef DIALOG_TRIGGER_PARSER_H_
#define DIALOG_TRIGGER_PARSER_H_

#include "aggregate.h"
#include "relational_ops.h"
#include "numeric.h"

namespace dialog {

struct parsed_trigger {
  aggregate_id agg;
  std::string field_name;
  relop_id relop;
  numeric threshold;
};

struct trigger_lex_token {
  trigger_lex_token(int i, const std::string& val)
      : id(i),
        value(val) {
  }

  int id;
  std::string value;
};

class trigger_lexer {
 public:
  static const int INVALID = -2;
  static const int END = -1;
  // Parentheses
  static const int LEFT = 1;
  static const int RIGHT = 2;
  // Operators
  static const int OPERATOR = 3;
  // Operand
  static const int OPERAND = 4;

  trigger_lexer() {
  }

  trigger_lexer(const std::string& exp) {
    str(exp);
  }

  void str(const std::string& exp) {
    stream_.str(exp);
  }

  size_t pos() {
    return stream_.tellg();
  }

  std::string str() {
    return stream_.str();
  }

  const trigger_lex_token next_token() {
    while (iswspace(stream_.peek()))
      stream_.get();

    char c = stream_.get();
    switch (c) {
      case EOF:
        return trigger_lex_token(END, "");
      case '(':
        return trigger_lex_token(LEFT, "(");
      case ')':
        return trigger_lex_token(RIGHT, ")");
      case '=': {
        if (stream_.get() != '=')
          THROW(parse_exception,
                "Invalid token starting with =; did you mean ==?");
        return trigger_lex_token(OPERATOR, "==");
      }
      case '<': {
        if (stream_.get() == '=')
          return trigger_lex_token(OPERATOR, "<=");
        stream_.unget();
        return trigger_lex_token(OPERATOR, "<");
      }
      case '>': {
        if (stream_.get() == '=')
          return trigger_lex_token(OPERATOR, ">=");
        stream_.unget();
        return trigger_lex_token(OPERATOR, ">");
      }
      default: {
        if (!opvalid(c))
          THROW(parse_exception, "All operands must conform to [a-zA-Z0-9_.]+");

        stream_.unget();
        std::string operand = "";
        while (opvalid(stream_.peek()))
          operand += (char) stream_.get();
        return trigger_lex_token(OPERAND, operand);
      }
    }
  }

 private:
  bool opvalid(int c) {
    return isalnum(c) || c == '.' || c == '_' || c == '-';
  }

  std::stringstream stream_;
};

class trigger_parser {
 public:
  trigger_parser(const std::string& exp, const schema_t& schema)
      : lex_(exp),
        schema_(schema) {
  }

  parsed_trigger parse() {
    parsed_trigger t = trig();
    if (lex_.next_token().id != trigger_lexer::END)
      THROW(parse_exception, "Parsing ended prematurely");
    return t;
  }

 private:
  parsed_trigger trig() {
    parsed_trigger t;
    auto tok = lex_.next_token();
    if (tok.id != trigger_lexer::OPERAND)
      THROW(parse_exception,
            "Could not parse aggregate function name " + tok.value);
    t.agg = aggregate_ops::string_to_agg(tok.value);
    if (lex_.next_token().id != trigger_lexer::LEFT)
      THROW(parse_exception, "Expected '(' after aggregate function name");
    tok = lex_.next_token();
    if (tok.id != trigger_lexer::OPERAND)
      THROW(parse_exception, "Could not parse field name " + tok.value);
    t.field_name = tok.value;
    if (lex_.next_token().id != trigger_lexer::RIGHT)
      THROW(parse_exception, "Expected ')' after field name");
    tok = lex_.next_token();
    if (tok.id != trigger_lexer::OPERATOR)
      THROW(parse_exception, "Could not parse relational operator" + tok.value);
    t.relop = relop_utils::str_to_op(tok.value);
    tok = lex_.next_token();
    if (tok.id != trigger_lexer::OPERAND)
      THROW(parse_exception, "Could not parse threshold value " + tok.value);
    t.threshold = numeric::parse(
        tok.value,
        t.agg == aggregate_id::D_CNT ?
            LONG_TYPE : schema_[t.field_name].type());
    return t;
  }

  trigger_lexer lex_;
  const schema_t& schema_;
};

}

#endif /* DIALOG_TRIGGER_PARSER_H_ */

#ifndef DIALOG_EXPRESSION_COMPILER_H_
#define DIALOG_EXPRESSION_COMPILER_H_

#include <vector>
#include <set>

#include "expression.h"
#include "minterm.h"
#include "compiled_predicate.h"
#include "compiled_expression.h"

namespace dialog {

class expression_compiler {
 public:
  template<typename schema_t>
  static compiled_expression compile(expression_t* exp,
                                     const schema_t& schema) {
    compiled_expression cexp;
    compile(cexp, exp, schema);
    return cexp;
  }

  template<typename schema_t>
  static void compile(compiled_expression& cexp, expression_t* exp,
                      const schema_t& schema) {
    switch (exp->id) {
      case expression_id::PREDICATE: {
        predicate_t *p = (predicate_t*) exp;
        exp_predicate(cexp, p, schema);
        break;
      }
      case expression_id::OR: {
        compiled_expression left, right;
        disjunction_t *d = (disjunction_t*) exp;
        compile(left, d->left, schema);
        compile(right, d->right, schema);
        exp_or(cexp, left, right);
        break;
      }
      case expression_id::AND: {
        compiled_expression left;
        conjunction_t *c = (conjunction_t*) exp;
        compile(left, c->left, schema);
        for (auto& m : left) {
          compiled_expression tmp;
          exp_and(tmp, c->right, m, schema);
          exp_or(cexp, cexp, tmp);
        }
        break;
      }
      default: {
        throw parse_exception("Unexpected node-type in compiling expression");
      }
    }
  }

  template<typename schema_t>
  static compiled_expression compile(const std::string& exp,
                                     const schema_t& schema) {
    compiled_expression cexp;
    parser p(exp);
    expression_t *e = p.parse();
    compile(cexp, e, schema);
    expression_utils::free_expression(e);
    return cexp;
  }

  template<typename schema_t>
  static void compile(compiled_expression& minterms, const std::string& exp,
                      const schema_t& schema) {
    parser p(exp);
    expression_t *e = p.parse();
    compile(minterms, e, schema);
    expression_utils::free_expression(e);
  }

 private:
  template<typename schema_t>
  static void exp_predicate(compiled_expression& o, predicate_t* p,
                            const schema_t& schema) {
    minterm m;
    m.add(compiled_predicate(*p, schema));
    o.insert(m);
  }
  static void exp_or(compiled_expression& o, const compiled_expression& l,
                     const compiled_expression& r) {
    std::set_union(l.begin(), l.end(), r.begin(), r.end(),
                   std::inserter(o, o.end()));
  }

  template<typename schema_t>
  static void exp_and(compiled_expression& a, const expression_t* e,
                      const minterm& m, const schema_t& schema) {
    minterm right = m;
    switch (e->id) {
      case expression_id::PREDICATE: {
        predicate_t *p = (predicate_t*) e;
        right.add(compiled_predicate(*p, schema));
        a.insert(right);
        break;
      }
      case expression_id::OR: {
        compiled_expression l, r;
        disjunction_t *d = (disjunction_t *) e;
        exp_and(l, d->left, m, schema);
        exp_and(r, d->right, m, schema);
        exp_or(a, l, r);
        break;
      }
      case expression_id::AND: {
        compiled_expression lor;
        conjunction_t *c = (conjunction_t *) e;
        exp_and(lor, c->left, m, schema);
        for (auto& lor_m : lor) {
          compiled_expression tmp;
          exp_and(tmp, c->right, lor_m, schema);
          exp_or(a, a, tmp);
        }
        break;
      }
      default: {
        throw parse_exception("Unexpected node type in compiling expression");
      }
    }
  }
};

}

#endif /* DIALOG_EXPRESSION_COMPILER_H_ */

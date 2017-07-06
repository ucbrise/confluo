#ifndef DIALOG_COMPILED_EXPRESSION_H_
#define DIALOG_COMPILED_EXPRESSION_H_

#include <set>

#include "minterm.h"

namespace dialog {

struct compiled_expression : public std::set<minterm> {
  inline bool test(const record_t& r) const {
    if (empty())
      return true;

    for (auto& p : *this)
      if (p.test(r))
        return true;

    return false;
  }
};

}

#endif /* DIALOG_COMPILED_EXPRESSION_H_ */

#ifndef DIALOG_COMPILED_EXPRESSION_H_
#define DIALOG_COMPILED_EXPRESSION_H_

#include <set>

#include "compiled_minterm.h"

namespace dialog {

struct compiled_expression : public std::set<compiled_minterm> {
  inline bool test(const record_t& r) const {
    if (empty())
      return true;

    for (auto& p : *this)
      if (p.test(r))
        return true;

    return false;
  }

  inline bool test(const schema_snapshot& snap, void* data) const {
      if (empty())
        return true;

      for (auto& p : *this)
        if (p.test(snap, data))
          return true;

      return false;
    }

  std::string to_string() const {
    std::string ret = "";
    size_t s = size();
    size_t i = 0;
    for (auto& p : *this) {
      ret += p.to_string();
      if (++i < s - 1)
        ret += " or ";
    }
    return ret;
  }
};

}

#endif /* DIALOG_COMPILED_EXPRESSION_H_ */

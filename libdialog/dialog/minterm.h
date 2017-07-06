#ifndef LIBDIALOG_DIALOG_MINTERM_H_
#define LIBDIALOG_DIALOG_MINTERM_H_

#include <vector>

#include "compiled_predicate.h"

namespace dialog {

struct minterm {
  typedef std::vector<compiled_predicate>::iterator iterator;
  typedef std::vector<compiled_predicate>::const_iterator const_iterator;

  minterm() = default;

  minterm(const minterm& other)
      : m_(other.m_) {
  }

  void add(const compiled_predicate& p) {
    m_.push_back(p);
  }

  bool operator<(const minterm& other) const {
    return to_string() < other.to_string();
  }

  minterm& operator=(const minterm& other) {
    m_ = other.m_;
    return *this;
  }

  std::string to_string() const {
    std::string s = ";";
    for (auto& p : m_) {
      s += p.to_string();
      s += ";";
    }
    return s;
  }

  inline bool test(const record_t& r) const {
    for (auto& p : m_)
      if (!p.test(r))
        return false;
    return true;
  }

  iterator begin() {
    return m_.begin();
  }

  iterator end() {
    return m_.end();
  }

  const_iterator begin() const {
    return m_.begin();
  }

  const_iterator end() const {
    return m_.end();
  }

 private:
  std::vector<compiled_predicate> m_;
};

}

#endif /* LIBDIALOG_DIALOG_MINTERM_H_ */

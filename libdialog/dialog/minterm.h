#ifndef DIALOG_MINTERM_H_
#define DIALOG_MINTERM_H_

#include <vector>

#include "compiled_predicate.h"

namespace dialog {

struct minterm {
  typedef std::set<compiled_predicate>::iterator iterator;
  typedef std::set<compiled_predicate>::const_iterator const_iterator;

  minterm() = default;

  minterm(const minterm& other)
      : m_(other.m_) {
  }

  inline void add(const compiled_predicate& p) {
    m_.insert(p);
  }

  inline bool test(const record_t& r) const {
    for (auto& p : m_)
      if (!p.test(r))
        return false;
    return true;
  }

  std::string to_string() const {
    std::string s = "";
    size_t size = m_.size();
    size_t i = 0;
    for (auto& p : m_) {
      s += p.to_string();
      if (++i < size - 1)
        s += " and ";
    }
    return s;
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

  inline bool operator<(const minterm& other) const {
    return to_string() < other.to_string();
  }

  minterm& operator=(const minterm& other) {
    m_ = other.m_;
    return *this;
  }

  size_t size() const {
    return m_.size();
  }

 private:
  std::set<compiled_predicate> m_;
};

}

#endif /* DIALOG_MINTERM_H_ */

#ifndef DIALOG_QUERY_PLANNER_H_
#define DIALOG_QUERY_PLANNER_H_

#include "compiled_expression.h"
#include "radix_tree.h"
#include "auxlog.h"
#include "query_plan.h"

namespace dialog {

class query_planner {
 public:
  typedef std::vector<index_filter> index_filters;
  typedef index_filters::iterator if_iterator;
  typedef index_filters::const_iterator const_if_iterator;

  static query_plan plan(const compiled_expression& expr,
                         const index_list_type& idx_list) {
    query_plan qp;
    for (const minterm& m : expr) {
      index_filters filters = get_filters(m);
      if (reduce_filters(filters))
        qp.push_back(minterm_plan(extract_min_filter(filters, idx_list), m));
    }
    return qp;
  }

 private:
  static index_filter extract_min_filter(const index_filters& filters,
                                         const index_list_type& idx_list) {
    const_if_iterator min_f;
    size_t min_count = UINT64_MAX;

    for (auto i = filters.begin(); i != filters.end(); i++) {
      uint64_t count;
      if ((count = idx_list.at(i->index_id())->approx_count(i->kbegin(),
                                                            i->kend()))) {
        min_count = count;
        min_f = i;
      }
    }

    return *min_f;
  }

  static index_filters get_filters(const minterm& m) {
    index_filters filters;
    for (const compiled_predicate& p : m) {
      if (p.is_indexed()) {
        index_filters f = p.idx_filters();
        append_filters(filters, f);
      }
    }
    return filters;
  }

  static void append_filters(index_filters& dst, index_filters& src) {
    if (dst.empty()) {
      dst = std::move(src);
    } else {
      dst.reserve(dst.size() + src.size());
      std::move(std::begin(src), std::end(src), std::back_inserter(dst));
      src.clear();
    }
  }

  static bool reduce_filters(index_filters& filters) {
    for (if_iterator i = filters.begin(); i != filters.end(); i++) {
      uint32_t index_id = i->index_id();
      byte_string kbegin = i->kbegin();
      byte_string kend = i->kend();
      for (const_if_iterator j = i + 1; j != filters.end();) {
        if (index_id == j->index_id()) {
          kbegin = std::max(kbegin, j->kbegin());
          kend = std::min(kend, j->kend());
          j = filters.erase(j);
        } else {
          j++;
        }
      }

      if (kbegin <= kend) {
        i->set_keys(kbegin, kend);
        i++;
      } else {
        return false;
      }
    }
    return true;
  }

};

}

#endif /* DIALOG_QUERY_PLANNER_H_ */

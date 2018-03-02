#ifndef EXAMPLES_TIMESERIES_DB_H_
#define EXAMPLES_TIMESERIES_DB_H_

#include <math.h>
#include "atomic_multilog.h"

namespace confluo {

class timeseries_db : public atomic_multilog {
 public:
  timeseries_db(const std::string& name, const std::vector<column_t>& schema,
                const std::string& path, const storage::storage_mode& storage,
                task_pool& task_pool)
      : atomic_multilog(name, schema, path, storage, task_pool) {
    // Index the timestamp column by calling add_index
    add_index("TIMESTAMP");
  }

  void get_range(std::vector<record_t>& out, int64_t ts1, int64_t ts2) {
    std::string expr = "TIMESTAMP >= " + std::to_string(ts1)
        + " && TIMESTAMP <= " + std::to_string(ts2);
    for (auto r = execute_filter(expr); r->has_more(); r->advance()) {
      out.push_back(r->get());
    }
  }

  record_t get_nearest_value(int64_t ts, bool direction) {
    std::string op;
    if (direction) {
      op = ">";
    } else {
      op = "<";
    }

    std::string exp = "TIMESTAMP " + op + " " + std::to_string(ts);
    auto r = execute_filter(exp);

    if (r->has_more()) {
      return r->get();
    } else {
      return record_t();
    }
  }

  void compute_diff(std::vector<record_t>& pts, uint64_t from_version,
                    uint64_t to_version) {
    for (uint64_t v = from_version; v < to_version; v += record_size()) {
      ro_data_ptr ptr;
      read(v, ptr);
      pts.push_back(record_t(v, ptr, record_size()));
    }
  }

  uint64_t get_version() {
    return rt_.get();
  }

};

}

#endif /* EXAMPLES_TIMESERIES_DB_H_ */

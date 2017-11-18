#ifndef DIALOG_TIMESERIES_DB_H_
#define DIALOG_TIMESERIES_DB_H_

#include <math.h>

#include <functional>
#include <numeric>
#include <thread>

#include "dialog_table.h"

using namespace ::dialog::monolog;
using namespace ::dialog::index;
using namespace ::dialog::monitor;
using namespace ::dialog::parser;
using namespace ::dialog::planner;
using namespace ::utils;

namespace dialog {

class timeseries_db : public dialog_table {
 public:
  timeseries_db(const std::string& table_name, 
          const std::vector<column_t>& table_schema,
          const std::string& path, const storage::storage_mode& storage,
          task_pool& task_pool) : 
      dialog_table(table_name, table_schema, path, storage, task_pool) {
          // Index the timestamp column by calling add_index
          add_index("TIMESTAMP");
  }

  size_t append(void* data) {
      return dialog_table::append(data);
  }

  void get_range(std::vector<data>& out, int64_t ts1, int64_t ts2) {
      std::string expr = "TIMESTAMP >= " + std::to_string(ts1) + 
          " && TIMESTAMP <= " + std::to_string(ts2);
      for(auto r = execute_filter(expr); !r.empty(); r = r.tail()) {
          out.push_back(r.head().at(0).value().to_data());
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
     
      if (!r.empty()) {
        return r.head();
      } else {
          return record_t();
      }
  }

  void compute_diff(std::vector<ro_data_ptr>& pts, 
          std::vector<uint64_t>& offsets,
          uint64_t from_version, uint64_t to_version) {

      size_t index = 0;
      uint64_t version;
      for (version = from_version; version <= to_version; 
              version++) {
          read(offsets[index], version, pts[index]);
          index++;
      }
  }

  uint64_t get_version() { 
      return rt_.get();
  }

};

}

#endif /* DIALOG_TIMESERIES_H_ */

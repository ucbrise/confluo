#ifndef DIALOG_TIMESERIES_DB_H_
#define DIALOG_TIMESERIES_DB_H_

#include <math.h>

#include <functional>
#include <numeric>
#include <thread>

#include "configuration_params.h"
#include "storage.h"
#include "monolog.h"
#include "read_tail.h"
#include "schema.h"
#include "record_batch.h"
#include "string_map.h"
#include "table_metadata.h"
#include "data_log.h"
#include "index_log.h"
#include "filter_log.h"
#include "alert_index.h"
#include "periodic_task.h"
#include "planner/query_planner.h"
#include "radix_tree.h"
#include "filter.h"
#include "trigger.h"
#include "exceptions.h"
#include "optional.h"
#include "parser/expression_compiler.h"
#include "parser/schema_parser.h"
#include "parser/trigger_compiler.h"
#include "time_utils.h"
#include "string_utils.h"
#include "task_pool.h"

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
      size_t filter_id = filters_.push_back(&filter_);
      std::string filter_name = "filter";
      filter_map_.put(filter_name, filter_id);

      for(auto r = query_filter(filter_name, ts1, ts2); !r.empty();
              r = r.tail()) {
          out.push_back(r.head().at(0).value().to_data());
      }
  }

  data get_nearest_value(int64_t ts, bool direction) {
      std::string op;
      if (direction) {
          op = ">";
      } else {
          op = "<";
      }

      std::string exp = "TIMESTAMP " + op + " " + std::to_string(ts);
      auto r = execute_filter(exp);
      
      return r.head().at(0).value().to_data();
  }

  void compute_diff(std::vector<const aggregated_reflog*>& pts, 
          uint64_t from_version, uint64_t to_version) {
      for (uint64_t version = from_version; version < to_version;
              version++) {
          pts.push_back(filter_.lookup(version));
      }
  }

 private:
  filter filter_;

};

}

#endif /* DIALOG_TIMESERIES_H_ */

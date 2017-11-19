#ifndef EXAMPLES_STREAM_CONSUMER_H_
#define EXAMPLES_STREAM_CONSUMER_H_

#include <math.h>
#include "atomic_multilog.h"

namespace confluo {

class stream_consumer : public atomic_multilog {
 public:
  stream_consumer(const std::string& table_name,
                const std::vector<column_t>& table_schema,
                const std::string& path, const storage::storage_mode& storage,
                task_pool& task_pool)
      : atomic_multilog(table_name, table_schema, path, storage, task_pool) {
        name = table_name;
        schema_t(cur_schema);
  }

  void read(std::string& _return, uint64_t offset, size_t record_size) {
      if (name.empty()) {
          throw illegal_state_exception("Must set table first");
      }
      ro_data_ptr data;
      atomic_multilog::read(offset, data);
      char* rbuf = (char*) data.get();
      
      _return = std::string(reinterpret_cast<const char*>(rbuf), 
              record_size);
  }

 private:
  std::string name;
  schema_t cur_schema;

};

}

#endif /* EXAMPLES_STREAM_CONSUMER_H_ */

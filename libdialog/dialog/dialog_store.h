#ifndef DIALOG_DIALOG_STORE_H_
#define DIALOG_DIALOG_STORE_H_

#include <cstdint>

#include "optional.h"
#include "storage.h"
#include "exceptions.h"
#include "dialog_table.h"
#include "task_pool.h"
#include "file_utils.h"

namespace dialog {

class dialog_store {
 public:
  dialog_store(const std::string& data_path)
      : data_path_(utils::file_utils::full_path(data_path)) {
    utils::file_utils::create_dir(data_path_);
  }

  int64_t add_table(const std::string& table_name,
                    const std::vector<column_t>& schema,
                    const storage::storage_id id) {
    optional<management_exception> ex;
    storage::storage_mode mode = storage::STORAGE_MODES[id];
    auto ret =
        mgmt_pool_.submit(
            [&table_name, &schema, &mode, &ex, this]() -> int64_t {
              size_t table_id;
              if (table_map_.get(table_name, table_id) != -1) {
                ex = management_exception("Table " + table_name + " already exists.");
                return INT64_C(-1);
              }
              utils::file_utils::create_dir(data_path_ + "/" + table_name);
              dialog_table* t = new dialog_table(table_name, schema, data_path_ + "/" + table_name,
                  mode, mgmt_pool_);
              table_id = tables_.push_back(t);
              if (table_map_.put(table_name, table_id) == -1) {
                ex = management_exception("Could not add table " + table_name + " to table map");
                return INT64_C(-1);
              }
              return table_id;
            });

    int64_t table_id = ret.get();
    if (ex.has_value())
      throw ex.value();
    return table_id;
  }

  int64_t get_table_id(const std::string& table_name) const {
    size_t table_id;
    if (table_map_.get(table_name, table_id) == -1) {
      throw management_exception("No such table " + table_name);
    }
    return static_cast<int64_t>(table_id);
  }

  dialog_table* get_table(const std::string& table_name) {
    return tables_[get_table_id(table_name)];
  }

  dialog_table* get_table(int64_t table_id) {
    if (table_id >= static_cast<int64_t>(tables_.size())) {
      throw management_exception(
          "No such table with id " + std::to_string(table_id));
    }
    return tables_[table_id];
  }

  int64_t remove_table(const std::string& table_name) {
    size_t table_id;
    if (table_map_.get(table_name, table_id) == -1) {
      throw management_exception("No such table " + table_name);
    }
    return table_map_.remove(table_name, table_id);
  }

  int64_t remove_table(int64_t table_id) {
    return remove_table(get_table(table_id)->get_name());
  }

 private:
  // Metadata
  std::string data_path_;

  // Manangement
  task_pool mgmt_pool_;

  // Tables
  monolog::monolog_exp2<dialog_table*> tables_;
  string_map<size_t> table_map_;
};

}

#endif /* DIALOG_DIALOG_STORE_H_ */

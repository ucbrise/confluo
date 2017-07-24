#ifndef LIBDIALOG_DIALOG_DIALOG_STORE_H_
#define LIBDIALOG_DIALOG_DIALOG_STORE_H_

#include <cstdint>

#include "storage.h"
#include "exceptions.h"
#include "dialog_table.h"

namespace dialog {

class dialog_store {
 public:
  dialog_store(const std::string& data_path)
      : data_path_(data_path) {
  }

  void add_table(const std::string& table_name,
                 const std::vector<column_t> schema,
                 const storage::storage_mode& mode) {
    dialog_table* t = new dialog_table(schema, data_path_ + "/" + table_name,
                                       mode, mgmt_pool_);
    size_t table_id = tables_.push_back(t);
    table_map_.put(table_name, table_id);
  }

  dialog_table* get_table(const std::string& table_name) {
    size_t table_id;
    if (!table_map_.get(table_name, table_id)) {
      THROW(management_exception, "No such table " + table_name);
    }
    return tables_[table_id];
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

#endif /* LIBDIALOG_DIALOG_DIALOG_STORE_H_ */

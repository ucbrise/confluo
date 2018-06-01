#include "confluo_store.h"

namespace confluo {

confluo_store::confluo_store(const std::string &data_path)
    : data_path_(utils::file_utils::full_path(data_path)) {
  utils::file_utils::create_dir(data_path_);
  // Note that this assumes a one-to-one relationship between the confluo_store and allocator
  ALLOCATOR.register_cleanup_callback(std::bind(&confluo_store::memory_management_callback, this));
}

int64_t confluo_store::create_atomic_multilog(const std::string &name,
                                              const std::vector<column_t> &schema,
                                              const storage_mode mode,
                                              const archival_mode a_mode) {
  optional<management_exception> ex;
  std::future<int64_t> ret = mgmt_pool_.submit(
      [&name, &schema, &mode, &a_mode, &ex, this]() -> int64_t {
        return create_atomic_multilog_task(name, schema, mode, a_mode, ex);
      });
  int64_t id = ret.get();
  if (ex.has_value())
    throw ex.value();
  return id;
}

int64_t confluo_store::create_atomic_multilog(const std::string &name,
                                              const std::string &schema,
                                              const storage_mode mode,
                                              const archival_mode a_mode) {
  optional<management_exception> ex;
  std::future<int64_t> ret = mgmt_pool_.submit(
      [&name, &schema, &mode, &a_mode, &ex, this]() -> int64_t {
        return create_atomic_multilog_task(name, schema, mode, a_mode, ex);
      });
  int64_t id = ret.get();
  if (ex.has_value())
    throw ex.value();
  return id;
}

int64_t confluo_store::load_atomic_multilog(const std::string &name) {
  optional<management_exception> ex;
  std::future<int64_t> ret = mgmt_pool_.submit(
      [&name, &ex, this]() -> int64_t {
        return load_atomic_multilog_task(name, ex);
      });
  int64_t id = ret.get();
  if (ex.has_value())
    throw ex.value();
  return id;
}

int64_t confluo_store::get_atomic_multilog_id(const std::string &name) const {
  size_t id;
  if (multilog_map_.get(name, id) == -1) {
    throw management_exception("No such atomic multilog " + name);
  }
  return static_cast<int64_t>(id);
}

atomic_multilog *confluo_store::get_atomic_multilog(const std::string &name) {
  return atomic_multilogs_[get_atomic_multilog_id(name)];
}

atomic_multilog *confluo_store::get_atomic_multilog(int64_t id) {
  if (id >= static_cast<int64_t>(atomic_multilogs_.size())) {
    throw management_exception(
        "No such atomic multilog with id " + std::to_string(id));
  }
  return atomic_multilogs_[id];
}

int64_t confluo_store::remove_atomic_multilog(const std::string &name) {
  size_t id;
  if (multilog_map_.get(name, id) == -1) {
    throw management_exception("No such atomic multilog " + name);
  }
  return multilog_map_.remove(name, id);
}

int64_t confluo_store::remove_atomic_multilog(int64_t id) {
  return remove_atomic_multilog(get_atomic_multilog(id)->get_name());
}

void confluo_store::memory_management_task() {
  if (ALLOCATOR.memory_utilization() >= configuration_params::MAX_MEMORY()) {
    for (size_t id = 0; id < atomic_multilogs_.size(); id++) {
      // TODO how aggressively to archive and should multilogs with archival OFF be archived?
      atomic_multilogs_.get(id)->archive();
    }
  }
}

void confluo_store::memory_management_callback() {
  for (size_t id = 0; id < atomic_multilogs_.size(); id++) {
    // TODO how aggressively to archive and should multilogs with archival OFF be archived?
    // Currently, yes they are forcibly archived.
    atomic_multilogs_.get(id)->archive();
  }
}

int64_t confluo_store::create_atomic_multilog_task(const std::string &name,
                                                   const std::vector<column_t> &schema,
                                                   const storage::storage_mode mode,
                                                   const archival::archival_mode a_mode,
                                                   optional<management_exception> &ex) {
  size_t id;
  if (multilog_map_.get(name, id) != -1) {
    ex = management_exception("Table " + name + " already exists.");
    return INT64_C(-1);
  }
  utils::file_utils::create_dir(data_path_ + "/" + name);
  atomic_multilog *t = new atomic_multilog(name, schema, data_path_ + "/" + name,
                                           mode, a_mode, mgmt_pool_);
  id = atomic_multilogs_.push_back(t);
  if (multilog_map_.put(name, id) == -1) {
    ex = management_exception(
        "Could not add atomic multilog " + name + " to atomic multilog map");
    return INT64_C(-1);
  }
  return static_cast<int64_t>(id);
}

int64_t confluo_store::create_atomic_multilog_task(const std::string &name,
                                                   const std::string &schema,
                                                   const storage::storage_mode mode,
                                                   const archival::archival_mode a_mode,
                                                   optional<management_exception> &ex) {
  size_t id;
  if (multilog_map_.get(name, id) != -1) {
    ex = management_exception("Table " + name + " already exists.");
    return INT64_C(-1);
  }
  utils::file_utils::create_dir(data_path_ + "/" + name);
  atomic_multilog *t = new atomic_multilog(name, schema, data_path_ + "/" + name,
                                           mode, a_mode, mgmt_pool_);
  id = atomic_multilogs_.push_back(t);
  if (multilog_map_.put(name, id) == -1) {
    ex = management_exception(
        "Could not add atomic multilog " + name + " to atomic multilog map");
    return INT64_C(-1);
  }
  return static_cast<int64_t>(id);
}

int64_t confluo_store::load_atomic_multilog_task(const std::string &name, optional<management_exception> &ex) {
  size_t id;
  if (multilog_map_.get(name, id) != -1) {
    ex = management_exception("Table " + name + " already loaded.");
    return INT64_C(-1);
  }
  atomic_multilog *t = new atomic_multilog(name, data_path_ + "/" + name, mgmt_pool_);
  id = atomic_multilogs_.push_back(t);
  if (multilog_map_.put(name, id) == -1) {
    ex = management_exception(
        "Could not add atomic multilog " + name + " to atomic multilog map");
    return INT64_C(-1);
  }
  return static_cast<int64_t>(id);
}

}
#ifndef CONFLUO_CONFLUO_STORE_H_
#define CONFLUO_CONFLUO_STORE_H_

#include <cstdint>

#include "optional.h"
#include "exceptions.h"
#include "atomic_multilog.h"
#include "file_utils.h"
#include "storage/storage.h"
#include "threads/task_pool.h"

using namespace ::utils;

namespace confluo {

/**
* Managment of an atomic multilog
*/
class confluo_store {
 public:
  /**
   * Constructor for creating confluo store
   * @param data_path The data path for the store
   */
  explicit confluo_store(const std::string& data_path);

  /**
   * Adds a atomic multilog to confluo store
   *
   * @param name The name of the atomic multilog
   * @param schema List of columns that define the schema
   * @param mode The storage mode of the confluo store
   * @return The id of the atomic multilog
   */
  int64_t create_atomic_multilog(const std::string& name,
                                 const std::vector<column_t>& schema,
                                 storage::storage_mode mode = storage::IN_MEMORY,
                                 archival::archival_mode a_mode = archival_mode::OFF);

  /**
   * Adds a atomic multilog to confluo store
   *
   * @param name The name of the atomic multilog
   * @param schema Schema string
   * @param mode The storage mode of the confluo store
   * @return The id of the atomic multilog
   */
  int64_t create_atomic_multilog(const std::string& name,
                                 const std::string& schema,
                                 storage::storage_mode mode = storage::IN_MEMORY,
                                 archival::archival_mode a_mode = archival_mode::OFF);

  /**
   * Loads an existing atomic multilog from disk
   *
   * @param name The name of the atomic multilog
   * @param mode Storage mode
   * @param a_mode Archival mode
   * @return The id of the atomic multilog
   */
  int64_t load_atomic_multilog(const std::string& name);

  /**
   * Gets the id of the atomic multilog
   * @param name The name of the atomic multilog
   * @return The id of the atomic multilog
   */
  int64_t get_atomic_multilog_id(const std::string& name) const;

  /**
   * Gets the specified atomic multilog
   * @param name The name of the atomic multilog
   * @return The atomic multilog that matches name
   */
  atomic_multilog* get_atomic_multilog(const std::string& name);

  /**
   * Gets the specified atomic multilog by id
   * @param id The id of the atomic multilog
   * @return The atomic multilog that matches the id
   */
  atomic_multilog* get_atomic_multilog(int64_t id);

  /**
   * Removes a atomic multilog specified by the name
   * @param name The name of the atomic multilog
   * @return The index of the removed atomic multilog or -1 if it doesn't exist
   */
  int64_t remove_atomic_multilog(const std::string& name);

  /**
   * Removes the atomic multilog specified by the id
   * @param id The id of the atomic multilog
   * @return The index of the removed atomic multilog or -1 if it doesn't exist
   */
  int64_t remove_atomic_multilog(int64_t id);

 private:
  /**
   * Memory management task
   */
  void memory_management_task();

  /**
   * Memory management callback
   */
  void memory_management_callback();

  /**
   * Task to create a new atomic multilog
   *
   * @param name The name of the atomic multilog
   * @param schema The schema of the atomic multilog
   * @param mode The storage mode of the atomic multilog
   * @param a_mode The archival mode of the atomic multilog
   * @param ex The exception if creation fails
   *
   * @return Identifier for the created atomic multilog
   */
  int64_t create_atomic_multilog_task(const std::string& name,
                                      const std::vector<column_t>& schema,
                                      storage::storage_mode mode,
                                      archival::archival_mode a_mode,
                                      optional<management_exception>& ex);

  /**
   * Task to create a new atomic multilog
   *
   * @param name The name of the atomic multilog
   * @param schema The schema of the atomic multilog
   * @param mode The storage mode of the atomic multilog
   * @param ex The exception if creation fails
   *
   * @return Identifier for the created atomic multilog
   */
  int64_t create_atomic_multilog_task(const std::string& name,
                                      const std::string& schema,
                                      storage::storage_mode mode,
                                      archival::archival_mode a_mode,
                                      optional<management_exception>& ex);

  /**
   * Task to load atomic multilog
   *
   * @param name The name of the atomic multilog
   * @param ex The exception if load fails
   * @return Identifier for the loaded atomic multilog
   */
  int64_t load_atomic_multilog_task(const std::string& name,
                                    optional<management_exception>& ex);

  // Metadata
  std::string data_path_;

  // Manangement
  task_pool mgmt_pool_;

  // Tables
  monolog::monolog_exp2<atomic_multilog*> atomic_multilogs_;
  string_map<size_t> multilog_map_;
};

}

#endif /* CONFLUO_CONFLUO_STORE_H_ */

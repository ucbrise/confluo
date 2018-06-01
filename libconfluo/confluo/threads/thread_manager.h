#ifndef CONFLUO_THREADS_THREAD_MANAGER_H_
#define CONFLUO_THREADS_THREAD_MANAGER_H_

#include <thread>
#include <sstream>

#include "conf/configuration_params.h"
#include "container/string_map.h"
#include "exceptions.h"
#include "logger.h"
#include "thread_utils.h"

namespace confluo {

/**
 * Information about the thread
 */
struct thread_info {
  /** The identifier of the thread */
  std::thread::id tid;
  /** Whether the thread is valid */
  atomic::type<bool> valid;
};

/**
 * Note: Not thread safe
 * Manages threads in the system, not thread safe
 */
class thread_manager {
 public:
  /**
   * Registers a thread to the manager
   * @return The id of the thread
   */
  static int register_thread();

  /**
   * Deregisters the thread
   * @return The id of the deregistered thread
   */
  static int deregister_thread();

  /**
   * Finds the thread
   * @return The id of the found thread
   */
  static int get_id();

  /**
   * Gets the maximum number of threads
   * @return The maximum number of threads
   */
  static int get_max_concurrency();

 private:
  /**
   * Initializes info for each thread
   *
   * @return A pointer to the thread info
   */
  static thread_info *init_thread_info();

  /**
   * Finds the given thread
   *
   * @return The identifier for the thread
   */
  static int find();

  /**
   * Sets the thread identifier
   *
   * @return Integer representing the index of the thread 
   */
  static int set();

  /**
   * Sets the thread to be invalid
   *
   * @param i The index of the thread
   */
  static void unset(int i);

  /** The maximum amount of threads Confluo supports */
  static int MAX_CONCURRENCY();

  /** The thread info */
  static thread_info *THREAD_INFO();
};

}

#endif /* CONFLUO_THREADS_THREAD_MANAGER_H_ */

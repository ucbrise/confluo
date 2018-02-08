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
  static int register_thread() {
    // De-register if already registered
    deregister_thread();
    int core_id = set();
    utils::thread_utils::set_self_core_affinity(core_id);
    return core_id;
  }

  /**
   * Deregisters the thread
   * @return The id of the deregistered thread
   */
  static int deregister_thread() {
    int core_id;
    if ((core_id = find()) != -1)
      unset(core_id);
    return core_id;
  }

  /**
   * Finds the thread
   * @return The id of the found thread
   */
  static int get_id() {
    return find();
  }

  /**
   * Gets the maximum number of threads
   * @return The maximum number of threads
   */
  static int get_max_concurrency() {
    return MAX_CONCURRENCY;
  }

  /**
   * Sets the maximum number of threads to a new value
   * @param max_concurrency The new maximum concurrency
   */
  static void set_max_concurrency(int max_concurrency) {
    MAX_CONCURRENCY = max_concurrency;
  }

 private:
  /**
   * Initializes info for each thread
   *
   * @return A pointer to the thread info
   */
  static thread_info* init_thread_info() {
    thread_info* tinfo = new thread_info[MAX_CONCURRENCY];
    for (int i = 0; i < MAX_CONCURRENCY; i++)
      atomic::init(&tinfo[i].valid, false);
    return tinfo;
  }

  /**
   * Finds the given thread
   *
   * @return The identifier for the thread
   */
  static int find() {
    auto tid = std::this_thread::get_id();
    for (int i = 0; i < MAX_CONCURRENCY; i++) {
      if (atomic::load(&THREAD_INFO[i].valid) && THREAD_INFO[i].tid == tid) {
        return i;
      }
    }

    return -1;
  }

  /**
   * Sets the thread identifier
   *
   * @return Integer representing the index of the thread 
   */
  static int set() {
    auto tid = std::this_thread::get_id();
    bool expected = false;
    for (int i = 0; i < MAX_CONCURRENCY; i++) {
      if (atomic::strong::cas(&THREAD_INFO[i].valid, &expected, true)) {
        THREAD_INFO[i].tid = tid;
        return i;
      }
      expected = false;
    }
    return -2;
  }

  /**
   * Sets the thread to be invalid
   *
   * @param i The index of the thread
   */
  static void unset(int i) {
    atomic::store(&THREAD_INFO[i].valid, false);
  }

  /** The maximum amount of threads Confluo supports */
  static int MAX_CONCURRENCY;
  /** The thread info */
  static thread_info* THREAD_INFO;
};

/** The max concurrecy is specified by the configuration parameters */
int thread_manager::MAX_CONCURRENCY = configuration_params::MAX_CONCURRENCY;
/** Initializes the thread information */
thread_info* thread_manager::THREAD_INFO = thread_manager::init_thread_info();

}

#endif /* CONFLUO_THREADS_THREAD_MANAGER_H_ */

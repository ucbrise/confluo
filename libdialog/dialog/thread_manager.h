#ifndef DIALOG_THREAD_MANAGER_H_
#define DIALOG_THREAD_MANAGER_H_

#include <thread>
#include <sstream>

#include "exceptions.h"
#include "string_map.h"
#include "logger.h"
#include "thread_utils.h"
#include "configuration_params.h"

namespace confluo {

struct thread_info {
  std::thread::id tid;
  atomic::type<bool> valid;
};

// Not thread safe
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
  static thread_info* init_thread_info() {
    thread_info* tinfo = new thread_info[MAX_CONCURRENCY];
    for (int i = 0; i < MAX_CONCURRENCY; i++)
      atomic::init(&tinfo[i].valid, false);
    return tinfo;
  }

  static int find() {
    auto tid = std::this_thread::get_id();
    for (int i = 0; i < MAX_CONCURRENCY; i++) {
      if (atomic::load(&THREAD_INFO[i].valid) && THREAD_INFO[i].tid == tid) {
        return i;
      }
    }

    return -1;
  }

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

  static void unset(int i) {
    atomic::store(&THREAD_INFO[i].valid, false);
  }

  static int MAX_CONCURRENCY;
  static thread_info* THREAD_INFO;
};

int thread_manager::MAX_CONCURRENCY = configuration_params::MAX_CONCURRENCY;
thread_info* thread_manager::THREAD_INFO = thread_manager::init_thread_info();

}

#endif /* DIALOG_THREAD_MANAGER_H_ */

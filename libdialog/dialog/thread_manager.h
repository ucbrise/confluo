#ifndef DIALOG_THREAD_MANAGER_H_
#define DIALOG_THREAD_MANAGER_H_

#include <thread>
#include <sstream>

#include "exceptions.h"
#include "string_map.h"
#include "logger.h"
#include "configuration_params.h"

namespace dialog {

struct thread_info {
  std::thread::id tid;
  atomic::type<bool> valid;
};

// Not thread safe
class thread_manager {
 public:
  static int register_thread() {
    if (find() == -1)
      return set();
    return -1;
  }

  static int deregister_thread() {
    int id;
    if ((id = find()) != -1)
      unset(id);
    return id;
  }

  static int get_id() {
    return find();
  }

  static int get_max_concurrency() {
    return MAX_CONCURRENCY;
  }

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

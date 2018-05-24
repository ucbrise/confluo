#include "threads/thread_manager.h"

namespace confluo {

/** The max concurrecy is specified by the configuration parameters */
int thread_manager::MAX_CONCURRENCY = configuration_params::MAX_CONCURRENCY;
/** Initializes the thread information */
thread_info *thread_manager::THREAD_INFO = thread_manager::init_thread_info();

int thread_manager::register_thread() {
  // De-register if already registered
  deregister_thread();
  int core_id = set();
  utils::thread_utils::set_self_core_affinity(core_id);
  return core_id;
}

int thread_manager::deregister_thread() {
  int core_id;
  if ((core_id = find()) != -1)
    unset(core_id);
  return core_id;
}

int thread_manager::get_id() {
  return find();
}

int thread_manager::get_max_concurrency() {
  return MAX_CONCURRENCY;
}

void thread_manager::set_max_concurrency(int max_concurrency) {
  MAX_CONCURRENCY = max_concurrency;
}

thread_info *thread_manager::init_thread_info() {
  thread_info *tinfo = new thread_info[MAX_CONCURRENCY];
  for (int i = 0; i < MAX_CONCURRENCY; i++)
    atomic::init(&tinfo[i].valid, false);
  return tinfo;
}

int thread_manager::find() {
  auto tid = std::this_thread::get_id();
  for (int i = 0; i < MAX_CONCURRENCY; i++) {
    if (atomic::load(&THREAD_INFO[i].valid) && THREAD_INFO[i].tid == tid) {
      return i;
    }
  }

  return -1;
}

int thread_manager::set() {
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

void thread_manager::unset(int i) {
  atomic::store(&THREAD_INFO[i].valid, false);
}

}
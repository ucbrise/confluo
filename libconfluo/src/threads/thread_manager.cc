#include "threads/thread_manager.h"

namespace confluo {

int thread_manager::register_thread(thread_id_t thread_id) {
  // De-register if already registered
  deregister_thread(thread_id);
  int core_id = set(thread_id);
  utils::thread_utils::set_core_affinity(thread_id, core_id);
  return core_id;
}

int thread_manager::deregister_thread(thread_id_t thread_id) {
  int core_id;
  if ((core_id = find(thread_id)) != -1)
    unset(core_id);
  return core_id;
}

int thread_manager::get_id(thread_id_t thread_id) {
  return find(thread_id);
}

int thread_manager::get_max_concurrency() {
  return MAX_CONCURRENCY();
}

thread_info *thread_manager::init_thread_info() {
  auto *tinfo = new thread_info[MAX_CONCURRENCY()];
  for (int i = 0; i < thread_manager::MAX_CONCURRENCY(); i++)
    atomic::init(&tinfo[i].valid, false);
  return tinfo;
}

int thread_manager::find(thread_id_t thread_id ) {
  for (int i = 0; i < thread_manager::MAX_CONCURRENCY(); i++) {
    if (atomic::load(&THREAD_INFO()[i].valid) && THREAD_INFO()[i].tid == thread_id) {
      return i;
    }
  }

  return -1;
}

int thread_manager::set(thread_id_t thread_id) {
  bool expected = false;
  for (int i = 0; i < thread_manager::MAX_CONCURRENCY(); i++) {
    if (atomic::strong::cas(&THREAD_INFO()[i].valid, &expected, true)) {
      THREAD_INFO()[i].tid = thread_id;
      return i;
    }
    expected = false;
  }
  return -2;
}

void thread_manager::unset(int i) {
  atomic::store(&THREAD_INFO()[i].valid, false);
}

int thread_manager::MAX_CONCURRENCY() {
  static int concurrency = configuration_params::MAX_CONCURRENCY();
  return concurrency;
}

thread_info *thread_manager::THREAD_INFO() {
  static thread_info *info = thread_manager::init_thread_info();
  return info;
}

}
#ifndef DATASTORE_COORDINATOR_H_
#define DATASTORE_COORDINATOR_H_

#include <cstdint>
#include <chrono>
#include <thread>

#include "atomic.h"

namespace datastore {

struct snapshot {
  std::vector<uint64_t> tails;

  snapshot() = default;
  snapshot(size_t size)
      : tails(size, UINT64_MAX) {
  }
};

template<typename data_store>
class coordinator {
 public:
  static const uint64_t idle = UINT64_MAX;
  static const uint64_t started = UINT64_MAX & ~(UINT64_C(1) << 63);

  static const uint64_t CLEAR_MASK = ~(UINT64_C(1) << 63);

  coordinator(std::vector<data_store>& stores, uint64_t sleep_us)
      : run_(false),
        sleep_us_(sleep_us),
        stores_(stores) {
  }

  snapshot get_snapshot() {
    uint64_t id = snapshots_.size();
    while (snapshots_.size() != id + 1)
      std::this_thread::yield();
    return snapshots_.get(id);
  }

  bool start() {
    bool expected = false;
    if (atomic::strong::cas(&run_, &expected, true)) {
      snapshot_thread_ = std::thread([&] {
        while (true) {
          std::this_thread::sleep_for(sleep_us_);
          if (!atomic::load(&run_)) {
            return;
          }
          do_snapshot();
        }
      });
      return true;
    }
    return false;
  }

  bool stop() {
    bool expected = true;
    bool success = atomic::strong::cas(&run_, &expected, false);
    if (success)
      snapshot_thread_.join();
    return success;
  }

 private:
  void do_snapshot() {
    snapshot s(stores_.size());
    for (size_t i = 0; i < stores_.size(); i++)
      s.tails[i] = stores_[i].begin_snapshot();
    for (size_t i = 0; i < stores_.size(); i++)
      stores_[i].end_snapshot(s.tails[i]);
    snapshots_.push_back(s);
  }

  atomic::type<bool> run_;
  std::chrono::microseconds sleep_us_;
  monolog::monolog_relaxed<snapshot> snapshots_;
  std::vector<data_store>& stores_;

  std::thread snapshot_thread_;
};

}

#endif /* DATASTORE_COORDINATOR_H_ */

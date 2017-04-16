#ifndef DATASTORE_COORDINATOR_H_
#define DATASTORE_COORDINATOR_H_

#include <cstdint>
#include <chrono>
#include <thread>

#include "time_utils.h"
#include "monolog.h"
#include "atomic.h"
#include "logger.h"

namespace datastore {

struct snapshot {
  std::vector<uint64_t> tails;

  snapshot() = default;
  snapshot(size_t size)
      : tails(size, UINT64_MAX) {
  }

  snapshot(const snapshot& s) {
    tails = s.tails;
  }

  std::string to_string() {
    std::string out = "(";
    for (const uint64_t t : tails) {
      out += t + ",";
    }
    out.pop_back();
    out += ")";
    return out;
  }
};

template<typename data_store>
class coordinator {
 public:
  static const uint64_t MONITOR_SLEEP_US = 1000000;

  coordinator(std::vector<data_store>& stores, uint64_t sleep_us)
      : run_(false),
        monitor_(false),
        sleep_us_(sleep_us),
        stores_(stores) {
  }

  snapshot& get_snapshot() {
    uint64_t id = snapshots_.size();
    LOG_INFO<< "Waiting for snapshot ID " << id;
    while (snapshots_.size() != id + 1)
      std::this_thread::yield();
    snapshot& s = snapshots_.get(id);
    LOG_INFO<< "Got snapshot ID " << id << ": " << s.to_string();
    return s;
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

  bool start_monitor() {
    bool expected = false;
    if (atomic::strong::cas(&monitor_, &expected, true)) {
      monitor_thread_ = std::thread([&] {
        auto sleep_dur = std::chrono::microseconds(MONITOR_SLEEP_US);
        uint64_t start = utils::time_utils::cur_ms();
        while (true) {
          std::this_thread::sleep_for(sleep_dur);
          if (!atomic::load(&monitor_)) {
            return;
          }
          uint64_t now = utils::time_utils::cur_ms();
          size_t nsnapshots = snapshots_.size();
          double snapshot_rate = (nsnapshots * 1000.0) / (double)(now - start);
          LOG_INFO << nsnapshots << " snapshots in " << (now - start) <<
          " ms [" << snapshot_rate << " snapshots/s]";
        }
      });
      return true;
    }
    return false;
  }

  bool stop_monitor() {
    bool expected = true;
    bool success = atomic::strong::cas(&monitor_, &expected, false);
    if (success)
      monitor_thread_.join();
    return success;
  }

 private:
  void do_snapshot() {
    snapshot s(stores_.size());
    for (size_t i = 0; i < stores_.size(); i++)
      stores_[i].send_begin_snapshot();
    for (size_t i = 0; i < stores_.size(); i++)
      s.tails[i] = stores_[i].recv_begin_snapshot();

    snapshots_.push_back(s);

    for (size_t i = 0; i < stores_.size(); i++)
      stores_[i].send_end_snapshot(s.tails[i]);
    for (size_t i = 0; i < stores_.size(); i++)
      stores_[i].recv_end_snapshot();
  }

  atomic::type<bool> run_;
  atomic::type<bool> monitor_;
  std::chrono::microseconds sleep_us_;
  monolog::monolog_relaxed<snapshot> snapshots_;
  std::vector<data_store>& stores_;

  std::thread snapshot_thread_;
  std::thread monitor_thread_;
};

}

#endif /* DATASTORE_COORDINATOR_H_ */

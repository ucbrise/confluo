#ifndef DATASTORE_COORDINATOR_H_
#define DATASTORE_COORDINATOR_H_

#include <cstdint>

#include "atomic.h"

namespace datastore {

typedef uint8_t snapshot_state;

struct snapshot {
  std::vector<uint64_t> tails;
  uint64_t id;

  snapshot(size_t size)
      : tails(size, UINT64_MAX),
        id(UINT64_MAX) {
  }
};

template<typename data_store>
class coordinator {
 public:
  typedef uint64_t snapshot_state;
  static const snapshot_state STALE = UINT64_MAX;
  static const snapshot_state UPDATING = UINT64_MAX;

  coordinator(std::vector<data_store>& stores)
      : stores_(stores),
        snapshot_store_(cc_),
        snapshot_bytes_(stores.size() * sizeof(uint64_t)) {
  }

  void ordering_op() {
    cc_.end_write_op(cc_.start_write_op());
  }

  snapshot get_snapshot() {
    return perform_snapshot();
  }

 private:
  snapshot perform_snapshot() {
    snapshot s(stores_.size());
    s.id = snapshot_store_.append(
        [&s, this](uint8_t*& data, size_t& length)-> void {
          for (size_t i = 0; i < stores_.size(); i++) {
            s.tails[i] = stores_[i].begin_snapshot();
          }
          data = (uint8_t*) &s.tails[0];
          length = snapshot_bytes_;
        });

    for (size_t i = 0; i < stores_.size(); i++)
      stores_[i].end_snapshot();

    return s;
  }

  size_t snapshot_bytes_;
  atomic::type<snapshot_state> state_;
  std::vector<data_store>& stores_;
  write_stalled cc_;
  dependent::log_store<in_memory, write_stalled> snapshot_store_;
};

}

#endif /* DATASTORE_COORDINATOR_H_ */

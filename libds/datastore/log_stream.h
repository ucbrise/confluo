#ifndef DATASTORE_LOG_STREAM_H_
#define DATASTORE_LOG_STREAM_H_

#include "monolog.h"

namespace datastore {
namespace stream {

typedef monolog::monolog_relaxed<uint64_t, 24> stream_type;

// Allows all data to pass
struct default_filter {
  bool operator()(uint64_t id, uint8_t* data, size_t len) {
    return true;
  }

  template<typename T>
  bool operator()(uint64_t id, const T& data) {
    return true;
  }
};

template<typename filter = default_filter>
class log_stream {
 public:
  static filter DEFAULT_FILTER;
  log_stream()
      : filter_(DEFAULT_FILTER) {
  }

  log_stream(filter& f)
      : filter_(f) {
  }

  void update(uint64_t id, uint8_t* data, size_t len) {
    if (filter_(id, data, len))
      stream_.push_back(id);
  }

  template<typename T>
  void update(uint64_t id, const T& data) {
    if (filter_(id, data))
      stream_.push_back(id);
  }

  const stream_type& cstream() const {
    return stream_;
  }

  stream_type& stream() {
    return stream_;
  }

 private:
  stream_type stream_;
  filter& filter_;
};

template<typename filter>
filter log_stream<filter>::DEFAULT_FILTER = filter();

}
}

#endif /* DATASTORE_LOG_STREAM_H_ */

#ifndef SLOG_TOKENS_H_
#define SLOG_TOKENS_H_

#include <vector>
#include <cstdint>

namespace slog {

struct token_t {
 public:
  token_t(uint32_t index_id, unsigned char* data) {
    index_id_ = index_id;
    data_ = *(uint64_t*) data;
  }

  token_t(uint32_t index_id, uint64_t data) {
    index_id_ = index_id;
    data_ = data;
  }

  uint32_t index_id() {
    return index_id_;
  }

  uint64_t data() {
    return data_;
  }

  void update_data(uint64_t data) {
    data_ = data;
  }

 private:
  uint32_t index_id_;
  uint64_t data_;
};

typedef std::vector<token_t> token_list;

}

#endif /* SLOG_TOKENS_H_ */

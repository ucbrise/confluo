#ifndef SLOG_TOKENS_H_
#define SLOG_TOKENS_H_

namespace slog {

typedef struct {
  uint32_t index_id;
  unsigned char* data;
  size_t len;
} token_t;

typedef std::vector<token_t> token_list;

}

#endif /* SLOG_TOKENS_H_ */

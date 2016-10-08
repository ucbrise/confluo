#ifndef SLOG_FILTEROPS_H_
#define SLOG_FILTEROPS_H_

namespace slog {

struct basic_filter {
  // bool negate; // TODO Add negation
  uint32_t index_id;
  unsigned char* token_prefix;
  uint32_t token_prefix_len;
};

typedef std::vector<basic_filter> filter_conjunction;
typedef std::vector<filter_conjunction> filter_disjunction;
typedef filter_disjunction filter_query;

}

#endif /* SLOG_FILTEROPS_H_ */

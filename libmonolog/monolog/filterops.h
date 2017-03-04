#ifndef MONOLOG_FILTEROPS_H_
#define MONOLOG_FILTEROPS_H_

namespace monolog {

struct basic_filter {
 public:
  basic_filter() {
    index_id_ = 0;
    token_beg_ = 0;
    token_end_ = 0;
  }

  basic_filter(uint32_t index_id, unsigned char* token_beg,
               unsigned char* token_end) {
    index_id_ = index_id;
    token_beg_ = *(uint64_t*) token_beg;
    token_end_ = *(uint64_t*) token_end;
  }

  basic_filter(uint32_t index_id, uint64_t token_beg, uint64_t token_end) {
    index_id_ = index_id;
    token_beg_ = token_beg;
    token_end_ = token_end;
  }

  basic_filter(uint32_t index_id, unsigned char* token)
      : basic_filter(index_id, token, token) {
  }

  basic_filter(uint32_t index_id, uint64_t token)
      : basic_filter(index_id, token, token) {
  }

  uint32_t index_id() {
    return index_id_;
  }

  uint64_t token_beg() {
    return token_beg_;
  }

  uint64_t token_end() {
    return token_end_;
  }

 private:
  uint32_t index_id_;
  uint64_t token_beg_;
  uint64_t token_end_;
  /* TODO: Add negation */
};

typedef std::vector<basic_filter> filter_conjunction;
typedef std::vector<filter_conjunction> filter_disjunction;
typedef filter_disjunction filter_query;

}

#endif /* MONOLOG_FILTEROPS_H_ */

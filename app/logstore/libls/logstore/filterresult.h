#ifndef MONOLOG_FILTERITERATOR_H_
#define MONOLOG_FILTERITERATOR_H_

#include <cstdint>
#include <iterator>

namespace monolog {

typedef std::iterator<std::input_iterator_tag, uint64_t, uint64_t, const uint64_t*, uint64_t> __input_iterator;

class filter_result {
 public:
  class filter_iterator : public __input_iterator {
   public:
    typedef uint64_t value_type;
    typedef uint64_t difference_type;
    typedef const uint64_t* pointer;
    typedef uint64_t reference;

    filter_iterator() {
      res_ = NULL;

      cur_tok_ = UINT64_MAX;
      cur_entry_list_ = NULL;
      cur_idx_ = -1;
    }

    filter_iterator(const filter_result *res) {
      res_ = res;

      cur_tok_ = res_->tok_min_;
      cur_entry_list_ = NULL;
      if (res_->index_ != NULL)
        cur_entry_list_ = res_->index_->at(cur_tok_);
      cur_idx_ = -1;

      advance();
    }

    filter_iterator(uint64_t tok, int64_t idx) {
      res_ = NULL;
      cur_entry_list_ = NULL;

      cur_tok_ = tok;
      cur_idx_ = idx;
    }

    filter_iterator(const filter_iterator& it) {
      res_ = it.res_;

      cur_tok_ = it.cur_tok_;
      cur_entry_list_ = it.cur_entry_list_;
      cur_idx_ = it.cur_idx_;
    }

    reference operator*() const {
      return cur_entry_list_->get(cur_idx_);
    }

    filter_iterator& operator++() {
      do {
        advance();
      } while (cur_tok_ != res_->tok_max_ + 1 &&
               cur_entry_list_->get(cur_idx_) >= res_->max_rid_);
      return *this;
    }

    filter_iterator operator++(int) {
      filter_iterator it = *this;
      ++(*this);
      return it;
    }

    bool operator==(filter_iterator other) const {
      return (cur_tok_ == other.cur_tok_) && (cur_idx_ == other.cur_idx_);
    }

    bool operator!=(filter_iterator other) const {
      return !(*this == other);
    }

    bool finished() {
      return cur_tok_ == res_->tok_max_ + 1;
    }

   private:
    void advance() {
      if (cur_tok_ == res_->tok_max_ + 1)
        return;
      cur_idx_++;
      if (cur_entry_list_ == NULL || static_cast<uint64_t>(cur_idx_) == cur_entry_list_->size()) {
        cur_idx_ = 0;
        while ((cur_entry_list_ = res_->index_->at(++cur_tok_)) == NULL
               && cur_tok_ <= res_->tok_max_);
      }
    }

    entry_list* cur_entry_list_;
    uint64_t cur_tok_;
    int64_t cur_idx_;
    const filter_result *res_;
  };

  filter_result() {
    index_ = NULL;
    tok_min_ = 1;
    tok_max_ = 0;
    max_rid_ = 0;
  }

  filter_result(const tiered_index_base* index, const uint64_t tok_min,
                const uint64_t tok_max, const uint64_t max_rid) {
    index_ = index;
    tok_min_ = tok_min;
    tok_max_ = tok_max;
    max_rid_ = max_rid;
  }

  filter_iterator begin() {
    return filter_iterator(this);
  }

  filter_iterator end() {
    return filter_iterator(tok_max_ + 1, 0);
  }

 private:
  const tiered_index_base* index_;
  uint64_t tok_min_;
  uint64_t tok_max_;
  uint64_t max_rid_;
};

}

#endif  // MONOLOG_FILTERITERATOR_H_

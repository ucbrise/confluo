#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include <cstdlib>
#include <iterator>

#ifdef COLLECT_LIST_STATS
struct _ls_stats {
  uint64_t num_allocs;
  uint64_t num_writes;
  uint64_t num_reads;
}ls_stats {0, 0, 0};

#define PRINT_STATS \
  fprintf(stderr, "Num Allocs=%llu\nNum Writes=%llu\nNum Reads=%llu\n",\
    ls_stats.num_allocs, ls_stats.num_writes, ls_stats.num_reads)

#define FLUSH_STATS ls_stats = {0, 0, 0}

#else
#define FLUSH_STATS
#define PRINT_STATS
#endif

#define NUM_VALS 14

typedef struct _block {
  uint32_t data[NUM_VALS];
  struct _block* next;
} Block;

class ll_iterator : public std::iterator<std::forward_iterator_tag, Block*> {
 public:
  ll_iterator(Block* block, size_t offset)
      : cur_block_(block),
        cur_offset_(offset) {
  }

  ll_iterator& operator++() {
    cur_offset_++;
    if (cur_offset_ % NUM_VALS == 0) {
      cur_block_ = cur_block_->next;
#ifdef COLLECT_LIST_STATS
      ls_stats.num_reads++;
#endif
    }
    return *this;
  }

  ll_iterator operator++(int) {
    ll_iterator tmp(*this);
    cur_offset_++;
    if (cur_offset_ % NUM_VALS == 0) {
      cur_block_ = cur_block_->next;
#ifdef COLLECT_LIST_STATS
      ls_stats.num_reads++;
#endif
    }
    return tmp;
  }

  bool operator ==(const ll_iterator& rhs) const {
    return cur_offset_ == rhs.cur_offset_;
  }

  bool operator !=(const ll_iterator& rhs) const {
    return cur_offset_ != rhs.cur_offset_;
  }

  uint32_t& operator*() const {
    return cur_block_->data[cur_offset_ % NUM_VALS];
  }

  uint32_t& operator->() const {
    return cur_block_->data[cur_offset_ % NUM_VALS];
  }

 private:
  Block* cur_block_;
  uint32_t cur_offset_;
};

class LinkedList {
 public:
  typedef ll_iterator iterator;

  LinkedList() {
    head_ = new Block;
#ifdef COLLECT_LIST_STATS
    ls_stats.num_allocs++;
#endif
    head_->next = NULL;
    tail_ = head_;
    size_ = 0;
  }

  // Get size
  size_t size() {
    return size_;
  }

  // Insert element at end
  void push_back(uint32_t i) {
    uint32_t block_offset = size_ % NUM_VALS;
    if (size_ && block_offset == 0) {
      // Allocate a new block
      Block *new_block = new Block;
#ifdef COLLECT_LIST_STATS
      ls_stats.num_allocs++;
#endif
      new_block->next = NULL;
      tail_->next = new_block;
      tail_ = new_block;
    }
    tail_->data[block_offset] = i;
#ifdef COLLECT_LIST_STATS
    ls_stats.num_reads++;
    ls_stats.num_writes++;
#endif
    size_++;
  }

  // Iterator pointing to beginning
  iterator begin() {
    return iterator(head_, 0);
  }

  // Iterator pointing to end
  iterator end() {
    return iterator(tail_, size_);
  }

 private:
  Block* head_;
  Block* tail_;
  size_t size_;
};

#endif /* LINKED_LIST_H_ */

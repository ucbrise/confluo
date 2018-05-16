#ifndef CONFLUO_CONTAINER_CURSOR_BATCHED_CURSOR_H_
#define CONFLUO_CONTAINER_CURSOR_BATCHED_CURSOR_H_

#include <vector>
#include <sys/types.h>
#include <cstdint>

namespace confluo {

/**
 * A batched cursor
 */
template<typename T>
class batched_cursor {
 public:
  /**
   * Constructs batched cursor with given batch size.
   *
   * @param batch_size The batch size
   */
  batched_cursor(size_t batch_size)
      : current_batch_pos_(0),
        current_batch_size_(0),
        current_batch_(batch_size) {
  }

  /**
   * Initializes the batched cursor. Any sub-class of batched cursor
   * must call this method in its constructor.
   */
  void init() {
    current_batch_size_ = load_next_batch();
  }

  virtual ~batched_cursor() {
  }

  /**
   * Get the current element at cursor.
   *
   * @return The current element at cursor.
   */
  T const& get() const {
    return current_batch_[current_batch_pos_];
  }

  /**
   * Get the current element at cursor.
   *
   * @return The current element at cursor.
   */
  T const& operator*() const {
    return get();
  }

  /**
   * Advance the cursor and load next element. If the cursor has has no more
   * elements to load, this operation does nothing.
   */
  void advance() {
    current_batch_pos_++;
    if (current_batch_pos_ >= current_batch_size_) {
      current_batch_size_ = load_next_batch();
      current_batch_pos_ = 0;
    }
  }

  /**
   * Checks if the cursor has more elements.
   * @return True if the cursor has more elements, false otherwise
   */
  bool has_more() const {
    return current_batch_pos_ < current_batch_size_;
  }

  /**
   * Checks if the cursor is empty.
   * @return True if the cursor is empty, false otherwise.
   */
  bool empty() const {
    return !has_more();
  }

 protected:
  /**
   * Populates the batch with elements for the next batch.
   *
   * @return The size of the loaded batch.
   */
  virtual size_t load_next_batch() = 0;

  /** Current position in batch **/
  size_t current_batch_pos_;
  /** Current batch size **/
  size_t current_batch_size_;
  /** Current batch **/
  std::vector<T> current_batch_;
};

}

#endif /* CONFLUO_CONTAINER_CURSOR_BATCHED_CURSOR_H_ */

#ifndef DIALOG_DIALOG_PTR_H_
#define DIALOG_DIALOG_PTR_H_

#include "atomic.h"
#include "exceptions.h"
#include "dialog_allocator.h"

namespace dialog {
namespace storage {

class ptr_constants {
 public:
  // Increments/decrements for first and second counters
  static const uint32_t FIRST_DELTA = 1U;
  static const uint32_t SECOND_DELTA = 1U << 16;
  static const uint32_t BOTH_DELTA = FIRST_DELTA + SECOND_DELTA;

  static const uint32_t FIRST_MASK = (1U << 16) - 1;
  static const uint32_t SECOND_SHIFT = 16;
};

const uint32_t ptr_constants::FIRST_DELTA;
const uint32_t ptr_constants::SECOND_DELTA;
const uint32_t ptr_constants::BOTH_DELTA;
const uint32_t ptr_constants::FIRST_MASK;
const uint32_t ptr_constants::SECOND_SHIFT;

/**
 * Should be created by a swappable_ptr<T> or by copy
 * of another read_only_ptr<T>. It holds a pointer to
 * a reference count of a swappable_ptr<T>.
 *
 * A nullptr reference count results in no deallocation.
 */
template<typename T>
class read_only_ptr {
 public:
  read_only_ptr() :
    ptr_(nullptr),
    offset_(0),
    ref_counts_(nullptr),
    uses_first_count_(true) {
  }

  read_only_ptr(T* ptr, atomic::type<uint32_t>* ref_counts, bool uses_first_count, size_t offset = 0) :
    ptr_(ptr),
    offset_(offset),
    ref_counts_(ref_counts) ,
    uses_first_count_(uses_first_count) {
  }

  read_only_ptr(const read_only_ptr<T>& other) :
    ptr_(other.ptr_),
    offset_(other.offset_),
    ref_counts_(other.ref_counts_),
    uses_first_count_(other.uses_first_count_) {
    if (ref_counts_ != nullptr) {
      uint32_t increment = uses_first_count_ ? ptr_constants::FIRST_DELTA : ptr_constants::SECOND_DELTA;
      atomic::faa(ref_counts_, increment);
    }
  }

  ~read_only_ptr() {
    decrement_compare_dealloc();
  }

  read_only_ptr& operator=(const read_only_ptr<T>& other) {
    // TODO potential infinite loop bug here
    init(other.ptr_, other.offset_, other.ref_counts_, other.uses_first_count_);
    if (ref_counts_ != nullptr) {
      uint32_t increment = uses_first_count_ ? ptr_constants::FIRST_DELTA : ptr_constants::SECOND_DELTA;
      atomic::faa(ref_counts_, increment);
    }
    return *this;
  }

  /**
   * Decrement counter for current pointer if any, and replace
   * with new pointer, offset and reference count.
   * @param ptr pointer to data
   * @param offset offset
   * @param ref_count
   * @param first_count
   */
  void init(T* ptr, size_t offset, bool uses_first_count, atomic::type<uint32_t>* ref_count) {
    decrement_compare_dealloc();
    ptr_ = ptr;
    offset_ = offset;
    ref_counts_ = ref_count;
    uses_first_count_ = uses_first_count;
  }

  T* get() const {
    return ptr_ ? ptr_ + offset_ : ptr_;
  }

  void set_offset(size_t offset) {
    offset_ = offset;
  }

 private:
  /**
   * Decrements the reference count if pointer and reference count are
   * not null. Deallocates the pointer if reference count reaches 0.
   */
  void decrement_compare_dealloc() {
    if (ptr_ != nullptr && ref_counts_ != nullptr) {
      uint32_t decrement = uses_first_count_ ? ptr_constants::FIRST_DELTA : ptr_constants::SECOND_DELTA;
      uint32_t prev_val = atomic::fas(ref_counts_, decrement);
      if (uses_first_count_) {
        prev_val &= ptr_constants::FIRST_MASK;
      } else {
        prev_val >>= ptr_constants::SECOND_SHIFT;
      }
      if (prev_val == 1) {
        ALLOCATOR.dealloc<T>(ptr_);
      }
    }
  }

  void decrement() {

  }

  T* ptr_;
  size_t offset_;
  atomic::type<uint32_t>* ref_counts_;
  // TODO: need to resolve space utilization since bool is an extra byte
  bool uses_first_count_;

};

// TODO: support multiple swaps, currently only supports 1
/**
 * The lifetime of a swappable pointer must exceed the lifetime of
 * any copies created by it, since copies rely on reference counts
 * allocated in the original pointer.
 *
 * After the internal pointer is set for the first time it can never
 * be null.
 */
template<typename T>
class swappable_ptr {

 public:

  swappable_ptr() :
    swappable_ptr(nullptr) {
  }

  /**
   * Constructor--initializes ref counts to 1 to
   * protect from premature deallocation by a copy
   * @param ptr pointer
   */
  swappable_ptr(T* ptr) :
    ref_counts_(ptr_constants::BOTH_DELTA),
    ptr_(ptr) {
  }

  ~swappable_ptr() {
    T* ptr = atomic::load(&ptr_);
    if (ptr != nullptr) {
      ptr_metadata* metadata = ptr_metadata::get(ptr);
      if (metadata->state_ == state_type::D_IN_MEMORY) {
        decrement_compare_dealloc_first();
      } else if (metadata->state_ == state_type::D_ARCHIVED) {
        decrement_compare_dealloc_second();
      }
    }
  }

  /**
   * Initialize pointer with a CAS operation.
   * Any load after initialization returns a non-null pointer
   * @param ptr pointer
   * @return true if initialization successful, otherwise false
   */
  bool atomic_init(T* ptr) {
    T* expected = nullptr;
    return atomic::strong::cas(&ptr_, &expected, ptr);
  }

  /**
   * Get the stored pointer
   * @return pointer
   */
  T* atomic_load() const {
    return atomic::load(&ptr_);
  }

  /**
   * Swap current pointer with a new one.
   * Current semantics only allow for a single swap.
   * This operation is safe against copies but not against
   * concurrent calls to swap_ptr.
   * @param new_ptr new pointer
   */
  void swap_ptr(T* new_ptr) {
    if (new_ptr == nullptr) {
      THROW(memory_exception, "Pointer cannot be null!");
    }
    T* old_ptr = atomic::load(&ptr_);
    ptr_metadata* metadata = ptr_metadata::get(new_ptr);
    atomic::store(&ptr_, new_ptr);

    // Deallocate the old pointer if there are no copies.
    decrement_compare_dealloc_first();
  }

  /**
   * Atomically write to ptr_[idx]
   * @param idx index to write to
   * @param val value to write
   * @return true if write successful, otherwise false
   */
  bool atomic_set(size_t idx, T val) {
    bool result = false;
    atomic::faa(&ref_counts_, ptr_constants::BOTH_DELTA);
    T* ptr = atomic::load(&ptr_);
    if (ptr != nullptr) {
      ptr[idx] = val;
      result = true;
    }
    atomic::fas(&ref_counts_, ptr_constants::BOTH_DELTA);
    return result;
  }

  /**
   * Atomically get ptr_[idx]
   * @param idx index to write to
   * @param val value to write
   * @return true if write successful, otherwise false
   */
  T& atomic_get(size_t idx) const {
    atomic::faa(&ref_counts_, ptr_constants::BOTH_DELTA);
    T* ptr = atomic::load(&ptr_);
    T& result = ptr[idx];
    atomic::fas(&ref_counts_, ptr_constants::BOTH_DELTA);
    return result;
  }

  /**
   * Create a read-only copy of the pointer and increment the
   * reference count. Return if the internal pointer is null.
   * @param copy reference to pointer to store in
   * @param offset offset into pointer
   */
  void atomic_copy(read_only_ptr<T>& copy, size_t offset = 0) const {
    // Increment both counters to guarantee that the loaded
    // pointer can't be deallocated if there are no copies.
    // Protects against loading before a swap begins and
    // making a copy after the swap finishes.
    atomic::faa(&ref_counts_, ptr_constants::BOTH_DELTA);
    T* ptr = atomic::load(&ptr_);

    if (ptr == nullptr) {
      // Decrement both ref counts (no possibility of them reaching 0 here)
      atomic::fas(&ref_counts_, ptr_constants::BOTH_DELTA);
      return;
    }

    // Correct the other counter and create the copy.
    ptr_metadata* metadata = ptr_metadata::get(ptr);
    if (metadata->state_ == state_type::D_IN_MEMORY) {
      // decrement other ref count (no possibility of it reaching 0 here)
      atomic::fas(&ref_counts_, ptr_constants::SECOND_DELTA);
      copy.init(ptr, offset, true, &ref_counts_);
      return;
    } else if (metadata->state_ == state_type::D_ARCHIVED) {
      // decrement other ref count (no possibility of it reaching 0 here)
      atomic::fas(&ref_counts_, ptr_constants::FIRST_DELTA);
      copy.init(ptr, offset, false, &ref_counts_);
      return;
    } else {
      THROW(memory_exception, "Unsupported pointer state during copy!");
    }
  }

 private:

  /**
   * Decrements the ref count and deallocates the pointer if it reaches 0.
   * @param ref_count reference count to decrement
   */
  void decrement_compare_dealloc_first() {
    if ((atomic::fas(&ref_counts_, 1U) & ptr_constants::FIRST_MASK) == 1) {
      ALLOCATOR.dealloc<T>(ptr_);
    }
  }

  void decrement_compare_dealloc_second() {
    if ((atomic::fas(&ref_counts_, ptr_constants::SECOND_DELTA) >> ptr_constants::SECOND_SHIFT) == 1) {
      ALLOCATOR.dealloc<T>(ptr_);
    }
  }

  // mutable reference counts to support logically const functions
  mutable atomic::type<uint32_t> ref_counts_;

  atomic::type<T*> ptr_;
};

}
}

#endif /* DIALOG_DIALOG_PTR_H_ */

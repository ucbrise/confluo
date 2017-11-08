#ifndef CONFLUO_STORAGE_PTR_H_
#define CONFLUO_STORAGE_PTR_H_

#include "atomic.h"
#include "encoded_ptr.h"
#include "exceptions.h"
#include "storage/allocator.h"

namespace confluo {
namespace storage {

/**
 * Pointer constants class. Contains constants for important values for 
 * pointers
 */
// TODO move to utility, abstract away increment and decrement functions
class ptr_constants {
 public:
  // Increments/decrements for first and second counters
  /** Increments or decrements for the first counter */
  static const uint32_t FIRST_DELTA = 1U;
  /** Increments or decrements for the second counter */
  static const uint32_t SECOND_DELTA = 1U << 16;
  /** Increments or decrements for both counters */
  static const uint32_t BOTH_DELTA = FIRST_DELTA + SECOND_DELTA;

  /** A mask for the first counter */
  static const uint32_t FIRST_MASK = (1U << 16) - 1;
  /** Shift amount for the second counter */
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
  read_only_ptr()
    : enc_ptr_(),
      ref_counts_(nullptr) {
  }

  read_only_ptr(encoded_ptr<T> enc_ptr, atomic::type<uint32_t>* ref_counts)
    : enc_ptr_(enc_ptr),
      ref_counts_(ref_counts) {
  }

  read_only_ptr(const read_only_ptr<T>& other)
      : enc_ptr_(other.enc_ptr_),
        ref_counts_(other.ref_counts_) {
    if (ref_counts_ != nullptr) {
      bool uses_first_count = ptr_metadata::get(enc_ptr_.internal_ptr())->state_ == state_type::D_IN_MEMORY;
      uint32_t increment = uses_first_count ? ptr_constants::FIRST_DELTA : ptr_constants::SECOND_DELTA;
      atomic::faa(ref_counts_, increment);
    }
  }

  /**
   * Deallocates the read only pointer
   */
  ~read_only_ptr() {
    decrement_compare_dealloc();
  }

  /**
   * Assigns another read only pointer to this read only pointer
   *
   * @param other The other read only pointer
   *
   * @return This read only pointer which has the contents of the other
   * read only pointer
   */
  read_only_ptr& operator=(const read_only_ptr<T>& other) {
    // TODO potential infinite loop bug here
    init(other.enc_ptr_, other.ref_counts_);
    if (ref_counts_ != nullptr) {
      bool uses_first_count = ptr_metadata::get(enc_ptr_.internal_ptr())->state_ == state_type::D_IN_MEMORY;
      uint32_t increment = uses_first_count ? ptr_constants::FIRST_DELTA : ptr_constants::SECOND_DELTA;
      atomic::faa(ref_counts_, increment);
    }
    return *this;
  }

  /**
   * Decrement counter for current pointer if any, and replace
   * with new pointer, offset and reference count.
   * @param ptr pointer to data
   * @param offset The offset in bytes
   * @param ref_count The number of references
   * @param first_count The first count
   */
  void init(encoded_ptr<T> enc_ptr, atomic::type<uint32_t>* ref_count) {
    decrement_compare_dealloc();
    enc_ptr_ = enc_ptr;
    ref_counts_ = ref_count;
  }

  encoded_ptr<T> get() const {
    return enc_ptr_;
  }

 private:
  /**
   * Decrements the reference count if pointer and reference count are
   * not null. Deallocates the pointer if reference count reaches 0.
   */
  void decrement_compare_dealloc() {
    void* internal_ptr = enc_ptr_.internal_ptr();
    if (internal_ptr != nullptr && ref_counts_ != nullptr) {
      bool uses_first_count = ptr_metadata::get(internal_ptr)->state_ == state_type::D_IN_MEMORY;
      uint32_t decrement = uses_first_count ? ptr_constants::FIRST_DELTA : ptr_constants::SECOND_DELTA;
      uint32_t prev_val = atomic::fas(ref_counts_, decrement);
      if (uses_first_count) {
        prev_val &= ptr_constants::FIRST_MASK;
      } else {
        prev_val >>= ptr_constants::SECOND_SHIFT;
      }
      if (prev_val == 1) {
        ALLOCATOR.dealloc(internal_ptr);
      }
    }
  }

  encoded_ptr<T> enc_ptr_;
  atomic::type<uint32_t>* ref_counts_;

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
  /**
   * swappable_ptr
   */
  swappable_ptr() :
    swappable_ptr(nullptr) {
  }

  /**
   * Constructor--initializes ref counts to 1 to
   * protect from premature deallocation by a copy
   * @param ptr The pointer
   */
  swappable_ptr(encoded_ptr<T> ptr) :
    ref_counts_(ptr_constants::BOTH_DELTA),
    ptr_(ptr) {
  }

  /**
   * Deallocates the pointer
   */
  ~swappable_ptr() {
    encoded_ptr<T> ptr = atomic::load(&ptr_);
    if (ptr.internal_ptr() != nullptr) {
      ptr_metadata* metadata = ptr_metadata::get(ptr.internal_ptr());
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
   * @param ptr The pointer to initialize
   * @return True if initialization successful, false otherwise
   */
  bool atomic_init(encoded_ptr<T> ptr) {
    encoded_ptr<T> expected;
    return atomic::strong::cas(&ptr_, &expected, ptr);
  }

  /**
   * Get the stored pointer. It is unsafe to access the pointer
   * through this method since it may be deallocated. For safe
   * access create a read-only copy.
   * @return pointer
   */
  encoded_ptr<T> atomic_load() const {
    return atomic::load(&ptr_);
  }

  /**
   * Swap current pointer with a new one.
   * Current semantics only allow for a single swap.
   * This operation is safe against copies but not against
   * concurrent calls to swap_ptr.
   * @param new_ptr The new pointer
   */
  void swap_ptr(encoded_ptr<T> new_ptr) {
    if (new_ptr.internal_ptr() == nullptr) {
      THROW(memory_exception, "Pointer cannot be null!");
    }
    // Get old pointer
    encoded_ptr<T> old_ptr = atomic::load(&ptr_).internal_ptr();
    // Store new pointer
    atomic::store(&ptr_, new_ptr);
    // Deallocate old pointer if there are no copies.
    if ((atomic::fas(&ref_counts_, 1U) & ptr_constants::FIRST_MASK) == 1) {
      ALLOCATOR.dealloc(old_ptr.internal_ptr());
    }
  }

  /**
   * Atomically get ptr_[idx].
   * @param idx index to write to
   * @return value at index
   */
  T atomic_get_decode(size_t idx) const {
    atomic::faa(&ref_counts_, ptr_constants::BOTH_DELTA);
    T result = atomic::load(&ptr_).decode(idx);
    atomic::fas(&ref_counts_, ptr_constants::BOTH_DELTA);
    return result;
  }

  /**
   * Create a read-only copy of the pointer and increment the
   * reference count. Return if the internal pointer is null.
   * @param copy A reference to pointer to store in
   * @param offset The offset into pointer
   */
  void atomic_copy(read_only_ptr<T>& copy, size_t offset = 0) const {
    // Increment both counters to guarantee that the loaded
    // pointer can't be deallocated if there are no copies.
    // Protects against loading before a swap begins and
    // making a copy after the swap finishes.
    atomic::faa(&ref_counts_, ptr_constants::BOTH_DELTA);
    encoded_ptr<T> ptr(atomic::load(&ptr_));
    ptr.set_offset(offset);

    if (ptr.internal_ptr() == nullptr) {
      // Decrement both ref counts (no possibility of them reaching 0 here)
      atomic::fas(&ref_counts_, ptr_constants::BOTH_DELTA);
      return;
    }

    // Correct the other counter and create the copy.
    ptr_metadata* metadata = ptr_metadata::get(ptr.internal_ptr());
    if (metadata->state_ == state_type::D_IN_MEMORY) {
      // decrement other ref count (no possibility of it reaching 0 here)
      atomic::fas(&ref_counts_, ptr_constants::SECOND_DELTA);
      copy.init(ptr, &ref_counts_);
      return;
    } else if (metadata->state_ == state_type::D_ARCHIVED) {
      // decrement other ref count (no possibility of it reaching 0 here)
      atomic::fas(&ref_counts_, ptr_constants::FIRST_DELTA);
      copy.init(ptr, &ref_counts_);
      return;
    } else {
      THROW(memory_exception, "Unsupported pointer state during copy!");
    }
  }

 private:
  /**
   * Decrements the first ref count and deallocates the pointer if it reaches 0.
   */
  void decrement_compare_dealloc_first() {
    if ((atomic::fas(&ref_counts_, 1U) & ptr_constants::FIRST_MASK) == 1) {
      ALLOCATOR.dealloc(atomic::load(&ptr_).internal_ptr());
    }
  }

  /**
   * Decrements the second ref count and deallocates the pointer if it reaches 0.
   */
  void decrement_compare_dealloc_second() {
    if ((atomic::fas(&ref_counts_, ptr_constants::SECOND_DELTA) >> ptr_constants::SECOND_SHIFT) == 1) {
      ALLOCATOR.dealloc(atomic::load(&ptr_).internal_ptr());
    }
  }

  mutable atomic::type<uint32_t> ref_counts_; // mutable reference counts for logically const functions
  atomic::type<encoded_ptr<T>> ptr_;
};

}
}

#endif /* CONFLUO_STORAGE_PTR_H_ */

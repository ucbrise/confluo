#ifndef CONFLUO_STORAGE_SWAPPABLE_PTR_H_
#define CONFLUO_STORAGE_SWAPPABLE_PTR_H_

#include "allocator.h"
#include "atomic.h"
#include "reference_counts.h"
#include "storage_utils.h"

namespace confluo {
namespace storage {

/**
 * Should be created by a swappable_ptr<T> or by copy
 * of another read_only_ptr<T>. It holds a pointer to
 * a reference count of a swappable_ptr<T>.
 */
template<typename T>
class read_only_ptr {
 public:

  read_only_ptr()
      : ptr_(nullptr),
        offset_(0),
        ref_counts_(nullptr) {
  }

  /**
   * Constructor.
   * @param ptr pointer to data
   * @param offset offset into data
   * @param ref_counts reference counts of both pointer states
   */
  read_only_ptr(T* ptr, size_t offset, reference_counts* ref_counts)
      : ptr_(ptr),
        offset_(offset),
        ref_counts_(ref_counts) {
  }

  /**
   * Copy constructor. Increments reference count.
   * @param other other pointer
   */
  read_only_ptr(const read_only_ptr<T>& other)
      : ptr_(other.ptr_),
        offset_(other.offset_),
        ref_counts_(other.ref_counts_) {
    if (ref_counts_ != nullptr) {
      bool uses_first_count = ptr_metadata::get(ptr_)->state_ == state_type::D_IN_MEMORY;
      uses_first_count ? ref_counts_->increment_first() : ref_counts_->increment_second();
    }
  }

  /**
   * Assignment operator. Increments reference count.
   * @param other other pointer
   * @return this read only pointer
   */
  read_only_ptr& operator=(const read_only_ptr<T>& other) {
    // TODO potential infinite loop bug here
    init(other.ptr_, other.offset_, other.ref_counts_);
    if (ref_counts_ != nullptr) {
      bool uses_first_count = ptr_metadata::get(ptr_)->state_ == state_type::D_IN_MEMORY;
      uses_first_count ? ref_counts_->increment_first() : ref_counts_->increment_second();
    }
    return *this;
  }

  ~read_only_ptr() {
    decrement_compare_dealloc();
  }

  /**
   * Decrement counter for current pointer if any, and replace
   * with new pointer, offset and reference count.
   * @param ptr pointer to data
   * @param offset logical offset into data
   * @param ref_counts
   */
  void init(T* ptr, size_t offset = 0, reference_counts* ref_counts = nullptr) {
    decrement_compare_dealloc();
    ptr_ = ptr;
    offset_ = offset;
    ref_counts_ = ref_counts;
  }

  /**
   * Get pointer to data
   * @return pointer
   */
  T* get() const {
    return ptr_;
  }

  /**
   * Set data offset
   * @param offset offset into decoded representation
   */
  void set_offset(size_t offset) {
    offset_ = offset;
  }

 private:
  /**
   * Decrements the reference count if pointer and reference count are
   * not null. Destroys and deallocates the pointer if reference count reaches 0.
   */
  void decrement_compare_dealloc() {
    if (ptr_ != nullptr && ref_counts_ != nullptr) {
      auto aux = ptr_aux_block::get(ptr_metadata::get(ptr_));
      bool uses_first_count = aux.state_ == state_type::D_IN_MEMORY;
      if (uses_first_count && ref_counts_->decrement_first_and_compare()) {
        lifecycle_util<T>::destroy(ptr_);
        ALLOCATOR.dealloc(ptr_);
      } else if (!uses_first_count && ref_counts_->decrement_second_and_compare()) {
        lifecycle_util<T>::destroy(ptr_);
        ALLOCATOR.dealloc(ptr_);
      }
    }
  }

  T* ptr_; // pointer to data
  size_t offset_; // logical offset into decoded data
  reference_counts* ref_counts_; // pointer to reference counts stored in the swappable_ptr ancestor

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
  swappable_ptr()
     : ref_counts_(),
       ptr_(nullptr) {
  }

  /**
   * Constructor--initializes ref counts to 1 to
   * protect from premature deallocation by a copy
   * @param ptr pointer
   */
  swappable_ptr(T* ptr)
      : ref_counts_(),
        ptr_(ptr) {
  }

  /**
   * Move constructor. Not thread-safe.
   * @param other other swappable_ptr
   */
  swappable_ptr(swappable_ptr&& other) {
    ref_counts_ = other.ref_counts_;
    ptr_ = atomic::load(&other.ptr_);
    T* null = nullptr;
    atomic::store(&other.ptr_, nullptr);
  }

  /**
   * Move assignment operator. Not thread-safe.
   * @param other other swappable_ptr
   */
  swappable_ptr& operator=(swappable_ptr&& other) {
    ref_counts_ = other.ref_counts_;
    ptr_ = atomic::load(&other.ptr_);
    T* null = nullptr;
    atomic::store(&other.ptr_, null);
    return *this;
  }

  /**
   * Destructor. Delegates deallocation to
   * allocator if reference count drops to zero.
   */
  ~swappable_ptr() {
    T* ptr = atomic::load(&ptr_);
    if (ptr != nullptr) {
      auto aux = ptr_aux_block::get(ptr_metadata::get(ptr));
      if (aux.state_ == state_type::D_IN_MEMORY && ref_counts_.decrement_first_and_compare()) {
        destroy_dealloc(ptr);
      } else if (aux.state_ == state_type::D_ARCHIVED && ref_counts_.decrement_second_and_compare()) {
        destroy_dealloc(ptr);
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
   * Get the stored pointer. It is unsafe to access the pointer
   * through this method since it may be deallocated. For safe
   * access create a read-only copy.
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
    // Get old pointer
    T* old_ptr = atomic::load(&ptr_);
    // Store new pointer
    atomic::store(&ptr_, new_ptr);
    // Deallocate old pointer if there are no copies.
    if (ref_counts_.decrement_first_and_compare()) {
      destroy_dealloc(old_ptr);
    }
  }

  /**
   * Atomically get the value at the logical index idx.
   * @param idx logical index into decoded data
   * @return value at index
   */
  T atomic_get(size_t idx) const {
    ref_counts_.increment_both();
    T result = atomic::load(&ptr_);
    ref_counts_.decrement_both();
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
    ref_counts_.increment_both();
    T* ptr = atomic::load(&ptr_);

    if (ptr == nullptr) {
      // Decrement both ref counts (no possibility of them reaching 0 here)
      ref_counts_.decrement_both();
      return;
    }

    // Correct the other counter and create the copy.
    auto aux = ptr_aux_block::get(ptr_metadata::get(ptr));
    if (aux.state_ == state_type::D_IN_MEMORY) {
      // decrement other ref count (no possibility of it reaching 0 here)
      ref_counts_.decrement_second();
      copy.init(ptr, offset, &ref_counts_);
      return;
    } else if (aux.state_ == state_type::D_ARCHIVED) {
      // decrement other ref count (no possibility of it reaching 0 here)
      ref_counts_.decrement_first();
      copy.init(ptr, offset, &ref_counts_);
      return;
    } else {
      THROW(memory_exception, "Unsupported pointer state during copy!");
    }
  }

 private:
  /**
   * Decrements the first ref count, and destroys & deallocates the pointer if it reaches 0.
   */
  static void destroy_dealloc(T* ptr) {
    lifecycle_util<T>::destroy(ptr);
    ALLOCATOR.dealloc(ptr);
  }

  mutable reference_counts ref_counts_; // mutable reference counts for logically const functions
  atomic::type<T*> ptr_;
};

}
}

#endif /* CONFLUO_STORAGE_SWAPPABLE_PTR_H_ */

#ifndef DIALOG_DIALOG_PTR_H_
#define DIALOG_DIALOG_PTR_H_

#include "atomic.h"
#include "exceptions.h"
#include "dialog_allocator.h"

namespace dialog {
namespace memory {

template<typename T>
class read_only_ptr {
 public:
  read_only_ptr() :
    ptr_(nullptr),
    offset_(0),
    ref_count_(nullptr) {
  }

  read_only_ptr(T* ptr, atomic::type<uint64_t>* ref_count, size_t offset = 0) :
    ptr_(ptr),
    offset_(offset),
    ref_count_(ref_count) {
  }

  read_only_ptr(read_only_ptr<T>& other) {
    init(other.ptr_, other.ref_count_, other.offset_);
    atomic::faa(ref_count_, 1ULL);
  }

  ~read_only_ptr() {
    if (ptr_ != nullptr && atomic::fas(ref_count_, 1ULL) == 1) {
      ALLOCATOR.dealloc<T>(ptr_);
    }
  }

  void init(T* ptr, size_t offset, atomic::type<uint64_t>* ref_count) {
    ptr_ = ptr;
    offset_ = offset;
    ref_count_ = ref_count;
  }

  T* get() const {
    return ptr_ + offset_;
  }

 private:
  T* ptr_;
  size_t offset_;
  atomic::type<uint64_t>* ref_count_;

};

// TODO: support multiple swaps, currently only supports 1
/**
 * The lifetime of a swappable pointer must exceed the lifetime of
 * any copies created by it, since copies rely on reference counts
 * allocated in the original pointer.
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
    ref_count1_(1),
    ref_count2_(1),
    ptr_(ptr) {
  }

  ~swappable_ptr() {
    T* ptr = atomic::load(&ptr_);
    if (ptr != nullptr) {
      ptr_metadata* metadata = ptr_metadata::get_metadata(ptr);
      if (metadata->state_ == state_type::D_IN_MEMORY) {
        decrement_compare_dealloc(&ref_count1_);
      } else if (metadata->state_ == state_type::D_ARCHIVED) {
        decrement_compare_dealloc(&ref_count2_);
      }
    }
  }

  /**
   * Initialize pointer with a CAS operation.
   * Any load after initialization returns a non-null pointer
   * @param ptr pointer
   * @return the stored pointer
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
   * @param new_ptr
   */
  void swap_ptr(T* new_ptr) {
    if (new_ptr == nullptr) {
      THROW(memory_exception, "Pointer cannot be null!");
    }
    T* old_ptr = atomic::load(&ptr_);
    ptr_metadata* metadata = ptr_metadata::get_metadata(new_ptr);
    atomic::store(&ptr_, new_ptr);

    // Deallocate the old pointer if there are no copies.
    decrement_compare_dealloc(&ref_count1_);
  }

  /**
   * Create a read-only copy of the pointer and increment the
   * reference count. Return if the internal pointer is null.
   * @return copy
   */
  void atomic_copy(read_only_ptr<T>& copy, size_t offset = 0) const {
    // Increment both counters to guarantee that the loaded
    // pointer can't be deallocated if there are no copies.
    // Protects against loading before a swap begins and
    // making a copy after the swap finishes.
    size_t a = atomic::faa(&ref_count1_, 1ULL);
    atomic::faa(&ref_count2_, 1ULL);
    T* ptr = atomic::load(&ptr_);

    if (ptr == nullptr) {
      // Decrement both ref counts (no possibility of them reaching 0 here)
      atomic::fas(&ref_count1_, 1ULL);
      atomic::fas(&ref_count2_, 1ULL);
      return;
    }

    // Correct the other counter and create the copy.
    ptr_metadata* metadata = ptr_metadata::get_metadata(ptr);
    if (metadata->state_ == 0U) {
      // decrement other ref count (no possibility of it reaching 0 here)
      atomic::fas(&ref_count2_, 1ULL);
      copy.init(ptr, offset, &ref_count1_);
      return;
    } else if (metadata->state_ == 1U) {
      // decrement other ref count (no possibility of it reaching 0 here)
      atomic::fas(&ref_count1_, 1ULL);
      copy.init(ptr, offset, &ref_count2_);
      return;
    } else {
      THROW(memory_exception, "Unsupported pointer state during copy!");
    }
  }

 private:

  /**
   * Decrements the ref count and deallocates the pointer if it reaches 0.
   * @param ref_count reference count to decrement
   * @return value of the reference count immediately prior to the decrement
   */
  size_t decrement_compare_dealloc(atomic::type<uint64_t>* ref_count) {
    size_t rc;
    if ((rc = atomic::fas(ref_count, 1ULL)) == 1) {
      ALLOCATOR.dealloc<T>(ptr_);
    }
    return rc;
  }

  // mutable ref counts to support logically const functions
  mutable atomic::type<uint64_t> ref_count1_;
  mutable atomic::type<uint64_t> ref_count2_;

  atomic::type<T*> ptr_;
};

}
}

#endif /* DIALOG_DIALOG_PTR_H_ */

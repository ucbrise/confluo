#ifndef DIALOG_DIALOG_PTR_H_
#define DIALOG_DIALOG_PTR_H_

#include "atomic.h"
#include "exceptions.h"
#include "dialog_allocator.h"

namespace dialog {
namespace storage {

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
    ref_count_(nullptr) {
  }

  read_only_ptr(T* ptr, atomic::type<uint32_t>* ref_count, size_t offset = 0) :
    ptr_(ptr),
    offset_(offset),
    ref_count_(ref_count) {
  }

  read_only_ptr(const read_only_ptr<T>& other) {
    init(other.ptr_, other.offset_, other.ref_count_);
    if (ref_count_ != nullptr)
      atomic::faa(ref_count_, 1U);
  }

  ~read_only_ptr() {
    decrement_compare_dealloc();
  }

  read_only_ptr& operator=(const read_only_ptr<T>& other) {
    init(other.ptr_, other.offset_, other.ref_count_);
    if (ref_count_ != nullptr)
      atomic::faa(ref_count_, 1U);
    return *this;
  }

  /**
   * Decrement counter for current pointer if any, and replace
   * with new pointer, offset and reference count.
   * @param ptr pointer to data
   * @param offset offset
   * @param ref_count
   */
  void init(T* ptr, size_t offset, atomic::type<uint32_t>* ref_count) {
    decrement_compare_dealloc();
    ptr_ = ptr;
    offset_ = offset;
    ref_count_ = ref_count;
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
    if (ptr_ != nullptr && ref_count_ != nullptr && atomic::fas(ref_count_, 1U) == 1) {
      ALLOCATOR.dealloc<T>(ptr_);
    }
  }

  T* ptr_;
  size_t offset_;
  atomic::type<uint32_t>* ref_count_;

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
      ptr_metadata* metadata = ptr_metadata::get(ptr);
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
    decrement_compare_dealloc(&ref_count1_);
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
    atomic::faa(&ref_count1_, 1U);
    atomic::faa(&ref_count2_, 1U);
    T* ptr = atomic::load(&ptr_);

    if (ptr == nullptr) {
      // Decrement both ref counts (no possibility of them reaching 0 here)
      atomic::fas(&ref_count1_, 1U);
      atomic::fas(&ref_count2_, 1U);
      return;
    }

    // Correct the other counter and create the copy.
    ptr_metadata* metadata = ptr_metadata::get(ptr);
    if (metadata->state_ == state_type::D_IN_MEMORY) {
      // decrement other ref count (no possibility of it reaching 0 here)
      atomic::fas(&ref_count2_, 1U);
      copy.init(ptr, offset, &ref_count1_);
      return;
    } else if (metadata->state_ == state_type::D_ARCHIVED) {
      // decrement other ref count (no possibility of it reaching 0 here)
      atomic::fas(&ref_count1_, 1U);
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
   */
  void decrement_compare_dealloc(atomic::type<uint32_t>* ref_count) {
    if (atomic::fas(ref_count, 1U) == 1) {
      ALLOCATOR.dealloc<T>(ptr_);
    }
  }

  // mutable reference counts to support logically const functions
  mutable atomic::type<uint32_t> ref_count1_;
  mutable atomic::type<uint32_t> ref_count2_;

  atomic::type<T*> ptr_;
};

}
}

#endif /* DIALOG_DIALOG_PTR_H_ */

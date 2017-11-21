#ifndef CONFLUO_STORAGE_PTR_H_
#define CONFLUO_STORAGE_PTR_H_

#include "allocator.h"
#include "atomic.h"
#include "encoded_ptr.h"
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
  typedef typename encoded_ptr<T>::decoded_ptr decoded_ptr;

  read_only_ptr()
      : enc_ptr_(),
        offset_(0),
        ref_counts_(nullptr) {
  }

  read_only_ptr(encoded_ptr<T> enc_ptr, size_t offset, reference_counts* ref_counts)
      : enc_ptr_(enc_ptr),
        offset_(offset),
        ref_counts_(ref_counts) {
  }

  read_only_ptr(const read_only_ptr<T>& other)
      : enc_ptr_(other.enc_ptr_),
        offset_(other.offset_),
        ref_counts_(other.ref_counts_) {
    if (ref_counts_ != nullptr) {
      bool uses_first_count = ptr_metadata::get(enc_ptr_.internal_ptr())->state_ == state_type::D_IN_MEMORY;
      uses_first_count ? ref_counts_->increment_first() : ref_counts_->increment_second();
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
    init(other.enc_ptr_, other.offset_, other.ref_counts_);
    if (ref_counts_ != nullptr) {
      bool uses_first_count = ptr_metadata::get(enc_ptr_.internal_ptr())->state_ == state_type::D_IN_MEMORY;
      uses_first_count ? ref_counts_->increment_first() : ref_counts_->increment_second();
    }
    return *this;
  }

  /**
   * Decrement counter for current pointer if any, and replace
   * with new pointer, offset and reference count.
   * @param ptr encoded pointer to data
   * @param offset logical offset into data
   * @param ref_counts
   */
  void init(encoded_ptr<T> enc_ptr, size_t offset = 0, reference_counts* ref_counts = nullptr) {
    decrement_compare_dealloc();
    enc_ptr_ = enc_ptr;
    offset_ = offset;
    ref_counts_ = ref_counts;
  }

  encoded_ptr<T> get() const {
    return enc_ptr_;
  }

  void set_offset(size_t offset) {
    offset_ = offset;
  }

  // Forwarding encode/decode member functions over encoded_ptr<T> below.
  // This is necessary since the offset must be stored outside encoded_ptr<T>
  // to minimize the footprint of atomic operations in swappable_ptr<T>.

  void encode(size_t idx, T val) {
    enc_ptr_.encode(idx + offset_, val);
  }

  void encode(size_t idx, size_t len, const T* data) {
    enc_ptr_.encode(idx + offset_, len, data);
  }

  decoded_ptr decode_ptr(size_t idx = 0) const {
    return enc_ptr_.decode_ptr(idx + offset_);
  }

  T decode(size_t idx) const {
    return enc_ptr_.decode(idx + offset_);
  }

  void decode(T* buffer, size_t idx, size_t len) const {
    enc_ptr_.decode(buffer, idx + offset_, len);
  }

 private:
  /**
   * Decrements the reference count if pointer and reference count are
   * not null. Destroys and deallocates the pointer if reference count reaches 0.
   */
  void decrement_compare_dealloc() {
    void* internal_ptr = enc_ptr_.internal_ptr();
    if (internal_ptr != nullptr && ref_counts_ != nullptr) {
      bool uses_first_count = ptr_metadata::get(internal_ptr)->state_ == state_type::D_IN_MEMORY;
      if (uses_first_count && ref_counts_->decrement_first_and_compare()) {
        destructor_util<T>::destroy(internal_ptr);
        ALLOCATOR.dealloc(internal_ptr);
      } else if (!uses_first_count && ref_counts_->decrement_second_and_compare()) {
        destructor_util<T>::destroy(internal_ptr);
        ALLOCATOR.dealloc(internal_ptr);
      }
    }
  }

  encoded_ptr<T> enc_ptr_; // encoded pointer to data
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
  /**
   * swappable_ptr
   */
  swappable_ptr() :
    ref_counts_(),
    enc_ptr_() {
  }

  /**
   * Constructor--initializes ref counts to 1 to
   * protect from premature deallocation by a copy
   * @param ptr The pointer
   */
  swappable_ptr(encoded_ptr<T> ptr) :
      ref_counts_(),
      enc_ptr_(ptr) {
  }

  /**
   * Deallocates the pointer
   */
  ~swappable_ptr() {
    encoded_ptr<T> enc_ptr = atomic::load(&enc_ptr_);
    if (enc_ptr.internal_ptr() != nullptr) {
      ptr_metadata* metadata = ptr_metadata::get(enc_ptr.internal_ptr());
      if (metadata->state_ == state_type::D_IN_MEMORY && ref_counts_.decrement_first_and_compare()) {
        destroy_dealloc(enc_ptr.internal_ptr());
      } else if (metadata->state_ == state_type::D_ARCHIVED && ref_counts_.decrement_second_and_compare()) {
        destroy_dealloc(enc_ptr.internal_ptr());
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
    return atomic::strong::cas(&enc_ptr_, &expected, ptr);
  }

  /**
   * Get the stored pointer. It is unsafe to access the pointer
   * through this method since it may be deallocated. For safe
   * access create a read-only copy.
   * @return pointer
   */
  encoded_ptr<T> atomic_load() const {
    return atomic::load(&enc_ptr_);
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
    encoded_ptr<T> old_ptr = atomic::load(&enc_ptr_).internal_ptr();
    // Store new pointer
    atomic::store(&enc_ptr_, new_ptr);
    // Deallocate old pointer if there are no copies.
    if (ref_counts_.decrement_first_and_compare()) {
      destroy_dealloc(old_ptr);
    }
  }

  /**
   * Atomically get the decoded value at the logical index idx.
   * @param idx logical index into decoded data
   * @return value at index
   */
  T atomic_get_decode(size_t idx) const {
    ref_counts_.increment_both();
    T result = atomic::load(&enc_ptr_).decode(idx);
    ref_counts_.decrement_both();
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
    ref_counts_.increment_both();
    encoded_ptr<T> ptr = atomic::load(&enc_ptr_);

    if (ptr.internal_ptr() == nullptr) {
      // Decrement both ref counts (no possibility of them reaching 0 here)
      ref_counts_.decrement_both();
      return;
    }

    // Correct the other counter and create the copy.
    ptr_metadata* metadata = ptr_metadata::get(ptr.internal_ptr());
    if (metadata->state_ == state_type::D_IN_MEMORY) {
      // decrement other ref count (no possibility of it reaching 0 here)
      ref_counts_.decrement_second();
      copy.init(ptr, offset, &ref_counts_);
      return;
    } else if (metadata->state_ == state_type::D_ARCHIVED) {
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
  void destroy_dealloc(encoded_ptr<T> encoded_ptr) {
    void* internal_ptr = encoded_ptr.internal_ptr();
    destructor_util<T>::destroy(internal_ptr);
    ALLOCATOR.dealloc(internal_ptr);
  }

  mutable reference_counts ref_counts_; // mutable reference counts for logically const functions
  atomic::type<encoded_ptr<T>> enc_ptr_;
};

}
}

#endif /* CONFLUO_STORAGE_PTR_H_ */

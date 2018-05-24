#ifndef CONFLUO_STORAGE_SWAPPABLE_ENCODED_PTR_H_
#define CONFLUO_STORAGE_SWAPPABLE_ENCODED_PTR_H_

#include "allocator.h"
#include "atomic.h"
#include "encoded_ptr.h"
#include "reference_counts.h"
#include "storage_utils.h"

namespace confluo {
namespace storage {

/**
 * Should be created by a swappable_encoded_ptr<T> or by copy
 * of another read_only_encoded_ptr<T>. It holds a pointer to
 * a reference count of a swappable_encoded_ptr<T>.
 */
template<typename T>
class read_only_encoded_ptr {
 public:
  read_only_encoded_ptr()
      : enc_ptr_(),
        offset_(0),
        ref_counts_(nullptr) {
  }

  /**
   * Constructor.
   * @param enc_ptr pointer to encoded data
   * @param offset offset into data
   * @param ref_counts reference counts of both pointer states
   */
  read_only_encoded_ptr(encoded_ptr<T> enc_ptr, size_t offset, reference_counts* ref_counts)
      : enc_ptr_(enc_ptr),
        offset_(offset),
        ref_counts_(ref_counts) {
  }

  /**
   * Copy constructor. Increments reference count.
   * @param other other pointer
   */
  read_only_encoded_ptr(const read_only_encoded_ptr<T>& other)
      : enc_ptr_(other.enc_ptr_),
        offset_(other.offset_),
        ref_counts_(other.ref_counts_) {
    if (ref_counts_ != nullptr) {
      auto* metadata = ptr_metadata::get(enc_ptr_.ptr());
      bool uses_first_count = ptr_aux_block::get(metadata).state_ == state_type::D_IN_MEMORY;
      uses_first_count ? ref_counts_->increment_first() : ref_counts_->increment_second();
    }
  }

  /**
   * Assignment operator. Increments reference count.
   * @param other other pointer
   * @return this read only pointer
   */
  read_only_encoded_ptr& operator=(const read_only_encoded_ptr<T>& other) {
    // TODO potential infinite loop bug here
    init(other.enc_ptr_, other.offset_, other.ref_counts_);
    if (ref_counts_ != nullptr) {
      auto* metadata = ptr_metadata::get(enc_ptr_.ptr());
      bool uses_first_count = ptr_aux_block::get(metadata).state_ == state_type::D_IN_MEMORY;
      uses_first_count ? ref_counts_->increment_first() : ref_counts_->increment_second();
    }
    return *this;
  }

  ~read_only_encoded_ptr() {
    decrement_compare_dealloc();
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

  /**
   * Get encoded pointer to data
   * @return encoded pointer
   */
  encoded_ptr<T> get() const {
    return enc_ptr_;
  }

  /**
   * Set data offset
   * @param offset offset into decoded representation
   */
  void set_offset(size_t offset) {
    offset_ = offset;
  }

  // Forwarding encode/decode member functions over encoded_ptr<T> below.
  // This is necessary since the offset must be stored outside encoded_ptr<T>
  // to minimize the footprint of atomic operations in swappable_encoded_ptr<T>.

  /**
   * Encode value and store at index
   * @param idx index to store at
   * @param val value
   */
  void encode(size_t idx, T val) {
    enc_ptr_.encode(idx + offset_, val);
  }

  /**
   * Encode data and store starting at index
   * @param idx start index
   * @param len length of decoded data
   * @param data decoded data buffer
   */
  void encode(size_t idx, size_t len, const T* data) {
    enc_ptr_.encode(idx + offset_, len, data);
  }

  /**
   * Decode data at index
   * @param idx index
   * @return decoded data
   */
  T decode_at(size_t idx) const {
    return enc_ptr_.decode_at(idx + offset_);
  }

  /**
   * Decode pointer from index onwards.
   * @param idx start index
   * @return decoded pointer
   */
  decoded_ptr<T> decode(size_t idx = 0) const {
    return enc_ptr_.decode(idx + offset_);
  }

  /**
   * Decodes length bytes of pointer from
   * index onwards and stores in a buffer.
   * @param buffer buffer to store in
   * @param idx start index
   * @param len length of decoded data
   */
  void decode(T* buffer, size_t idx, size_t len) const {
    enc_ptr_.decode(buffer, idx + offset_, len);
  }

  /**
   * Decodes length bytes of pointer from index onwards.
   * @param idx start index
   * @param len length of decoded data
   * @return decoded pointer
   */
  std::unique_ptr<T> decode(size_t idx, size_t len) const {
    return enc_ptr_.decode(idx + offset_, len);
  }

  T operator[](size_t idx) {
    return decode_at(idx);
  }

 private:
  /**
   * Decrements the reference count if pointer and reference count are
   * not null. Destroys and deallocates the pointer if reference count reaches 0.
   */
  void decrement_compare_dealloc() {
    void* internal_ptr = enc_ptr_.ptr();
    if (internal_ptr != nullptr && ref_counts_ != nullptr) {
      auto* metadata = ptr_metadata::get(internal_ptr);
      bool uses_first_count = ptr_aux_block::get(metadata).state_ == state_type::D_IN_MEMORY;
      if (uses_first_count && ref_counts_->decrement_first_and_compare()) {
        lifecycle_util<T>::destroy(internal_ptr);
        ALLOCATOR.dealloc(internal_ptr);
      } else if (!uses_first_count && ref_counts_->decrement_second_and_compare()) {
        lifecycle_util<T>::destroy(internal_ptr);
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
class swappable_encoded_ptr {

 public:
  swappable_encoded_ptr()
     : ref_counts_(),
       enc_ptr_(encoded_ptr<T>()) {
  }

  /**
   * Move constructor. Not thread-safe.
   * @param other other swappable_ptr
   */
  swappable_encoded_ptr(swappable_encoded_ptr&& other) {
    ref_counts_ = other.ref_counts_;
    enc_ptr_ = atomic::load(&other.enc_ptr_);
    atomic::store(&other.enc_ptr_, nullptr);
  }

  /**
   * Move assignment operator. Not thread-safe.
   * @param other other swappable_ptr
   */
  swappable_encoded_ptr& operator=(swappable_encoded_ptr&& other) {
    ref_counts_ = other.ref_counts_;
    enc_ptr_ = atomic::load(&other.enc_ptr_);
    atomic::store(&other.enc_ptr_, encoded_ptr<T>());
    return *this;
  }

  /**
   * Constructor--initializes ref counts to 1 to
   * protect from premature deallocation by a copy
   * @param ptr The pointer
   */
  swappable_encoded_ptr(encoded_ptr<T> ptr) :
      ref_counts_(),
      enc_ptr_(ptr) {
  }

  ~swappable_encoded_ptr() {
    encoded_ptr<T> enc_ptr = atomic::load(&enc_ptr_);
    if (enc_ptr.ptr() != nullptr) {
      auto aux = ptr_aux_block::get(ptr_metadata::get(enc_ptr.ptr()));
      if (aux.state_ == state_type::D_IN_MEMORY && ref_counts_.decrement_first_and_compare()) {
        destroy_dealloc(enc_ptr.ptr());
      } else if (aux.state_ == state_type::D_ARCHIVED && ref_counts_.decrement_second_and_compare()) {
        destroy_dealloc(enc_ptr.ptr());
      }
    }
  }

  /**
   * Initialize pointer with a CAS operation.
   * @param ptr pointer to initialize as
   * @return true if initialization successful, otherwise false
   */
  bool atomic_init(encoded_ptr<T> ptr) {
    encoded_ptr<T> expected;
    return atomic::strong::cas(&enc_ptr_, &expected, ptr);
  }

  /**
   * Initialize pointer with a CAS operation.
   * @param ptr pointer
   * @return true if initialization successful, otherwise false
   */
  bool atomic_init(encoded_ptr<T> ptr, encoded_ptr<T> expected) {
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
    if (new_ptr.ptr() == nullptr) {
      THROW(memory_exception, "Pointer cannot be null!");
    }
    // Get old pointer
    encoded_ptr<T> old_ptr = atomic::load(&enc_ptr_).ptr();
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
    T result = atomic::load(&enc_ptr_).decode_at(idx);
    ref_counts_.decrement_both();
    return result;
  }

  /**
   * Create a read-only copy of the pointer and increment the
   * reference count. Return if the internal pointer is null.
   * @param copy A reference to pointer to store in
   * @param offset The offset into pointer
   */
  void atomic_copy(read_only_encoded_ptr<T>& copy, size_t offset = 0) const {
    // Increment both counters to guarantee that the loaded
    // pointer can't be deallocated if there are no copies.
    // Protects against loading before a swap begins and
    // making a copy after the swap finishes.
    ref_counts_.increment_both();
    encoded_ptr<T> ptr = atomic::load(&enc_ptr_);

    if (ptr.ptr() == nullptr) {
      // Decrement both ref counts (no possibility of them reaching 0 here)
      ref_counts_.decrement_both();
      return;
    }

    // Correct the other counter and create the copy.
    auto aux = ptr_aux_block::get(ptr_metadata::get(ptr.ptr()));
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
  static void destroy_dealloc(encoded_ptr<T> encoded_ptr) {
    void* internal_ptr = encoded_ptr.ptr();
    lifecycle_util<T>::destroy(internal_ptr);
    ALLOCATOR.dealloc(internal_ptr);
  }

  mutable reference_counts ref_counts_; // mutable reference counts for logically const functions
  atomic::type<encoded_ptr<T>> enc_ptr_;
};

}
}

#endif /* CONFLUO_STORAGE_SWAPPABLE_ENCODED_PTR_H_ */

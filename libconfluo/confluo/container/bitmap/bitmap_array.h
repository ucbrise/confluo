#ifndef CONFLUO_CONTAINER_BITMAP_BITMAP_ARRAY_H_
#define CONFLUO_CONTAINER_BITMAP_BITMAP_ARRAY_H_

#include <limits>

#include "bitmap.h"

namespace confluo {

/** 
 * Value reference class. Contains a reference to a bitmap array and 
 * supports pointer operations
 */
template<typename bitmap_array_impl>
class value_reference {
 public:
  /** Contains the position type for a bitmap array */
  typedef typename bitmap_array_impl::pos_type pos_type;
  /** Contains the type of value that the bitmap array holds */
  typedef typename bitmap_array_impl::value_type value_type;
  /** A reference to to the bitmap array */
  typedef typename bitmap_array_impl::reference reference;

  /**
   * Constructs a value reference from a bitmap array
   *
   * @param array The array reference to construct this value reference
   * @param pos The position type
   */
  value_reference(bitmap_array_impl* array, pos_type pos)
      : array_(array),
        pos_(pos) {
  }

  /**
   * Assigns another value type to this value reference
   *
   * @param val The val to set this value type to
   *
   * @return A pointer to this value reference
   */
  reference& operator=(value_type val) {
    array_->set(pos_, val);
    return *this;
  }

  /**
   * Assigns another value reference to this value reference
   *
   * @param ref The other reference
   *
   * @return This value reference
   */
  reference& operator=(const value_reference& ref) {
    return (*this) = value_type(ref);
  }

  /**
   * Gets the value type of this valuer reference
   *
   * @return The value type
   */
  operator value_type() const {
    return array_->get(pos_);
  }

  /**
   * Increments the value at the position by one
   *
   * @return This value reference with the incremented value
   */
  reference& operator++() {
    value_type val = array_->get(pos_);
    array_->set(pos_, val + 1);
    return *this;
  }

  /**
   * Gets the current value and increments it after
   *
   *
   * @return The non incremented value
   */
  value_type operator++(int) {
    value_type val = (value_type) *this;
    ++(*this);
    return val;
  }

  /**
   * Decrements the value at the given position by one
   *
   * @return The current value reference
   */
  value_reference& operator--() {
    value_type val = array_->get(pos_);
    array_->set(pos_, val - 1);
    return *this;
  }

  /**
   * Gets the decremented value
   *
   *
   * @return The decremented value type
   */
  value_type operator--(int) {
    value_type val = (value_type) *this;
    --(*this);
    return val;
  }

  /**
   * Increments the value type
   *
   * @param x The other value type
   *
   * @return A reference to the value type
   */
  reference& operator+=(const value_type x) {
    value_type val = array_->get(pos_);
    array_->set(pos_, val + 1);
    return *this;
  }

  /**
   * Decrements the value at a given position
   *
   * @param x The other value type
   *
   * @return This value reference that was decremented
   */
  reference& operator-=(const value_type x) {
    value_type val = array_->get(pos_);
    array_->set(pos_, val - 1);
    return *this;
  }

  /**
   * Performs an equality comparison with the passed in value reference
   *
   * @param x The other value reference
   *
   * @return True if the value references are equal, false otherwise
   */
  bool operator==(const value_reference& x) const {
    return value_type(*this) == value_type(x);
  }

  /**
   * Performs a less than comparison with the passed in value reference
   *
   * @param x The other value reference
   *
   * @return True if this value reference is less than the other value
   * reference, false otherwise
   */
  bool operator<(const value_reference& x) const {
    return value_type(*this) < value_type(x);
  }

  /**
   * Swaps two references
   *
   * @param lhs The first reference
   * @param rhs The second reference
   */
  friend void swap(reference& lhs, reference& rhs) {
    value_type temp = value_type(lhs);
    lhs = rhs;
    rhs = temp;
  }

  /**
   * Swaps two references
   *
   * @param lhs The first reference
   * @param rhs The second reference
   */
  friend void swap(reference lhs, reference rhs) {
    value_type temp = value_type(lhs);
    lhs = rhs;
    rhs = temp;
  }

  /**
   * Swaps two references
   *
   * @param lhs The first reference
   * @param rhs The second reference
   */
  friend void swap(reference lhs, value_type rhs) {
    value_type temp = value_type(lhs);
    lhs = rhs;
    rhs = temp;
  }

  /**
   * Swaps a value type and reference
   *
   * @param lhs The value type to swap
   * @param rhs The reference to swap
   */
  friend void swap(value_type lhs, reference rhs) {
    value_type temp = value_type(rhs);
    rhs = lhs;
    lhs = temp;
  }

 private:
  bitmap_array_impl* const array_;
  const pos_type pos_;
};

// Iterators
/**
 * Iterator through a bitmap array
 */
template<typename bitmap_array_impl>
class bitmap_array_iterator {
 public:
  /** The position type */
  typedef typename bitmap_array_impl::pos_type pos_type;

  /** The difference type */
  typedef typename bitmap_array_impl::difference_type difference_type;
  /** The value type */
  typedef typename bitmap_array_impl::value_type value_type;
  /** The bitmap pointer */
  typedef typename bitmap_array_impl::pointer pointer;
  /** The bitmap reference */
  typedef typename bitmap_array_impl::reference reference;
  /** The iterator category */
  typedef typename bitmap_array_impl::iterator_category iterator_category;

  /**
   * Constructs an empty bitmap array iterator
   */
  bitmap_array_iterator() {
    array_ = NULL;
    pos_ = 0;
  }

  /**
   * Constructs a bitmap array iterator
   *
   * @param array The bitmap array implementation
   * @param pos The current position of the iterator
   */
  bitmap_array_iterator(bitmap_array_impl* array, pos_type pos) {
    array_ = array;
    pos_ = pos;
  }

  /**
   * operator*
   *
   * @return Reference at the given position
   */
  reference operator*() const {
    return reference(array_, pos_);
  }

  /**
   * operator++ (prefix)
   *
   * @return Updated iterator
   */
  bitmap_array_iterator& operator++() {
    pos_++;
    return *this;
  }

  /**
   * operator++ (postfix)
   *
   * 
   *
   * @return Updated iterator
   */
  bitmap_array_iterator operator++(int) {
    bitmap_array_iterator it = *this;
    ++(*this);
    return it;
  }

  /**
   * operator-- (prefix)
   *
   * @return Updated iterator
   */
  bitmap_array_iterator& operator--() {
    pos_--;
    return *this;
  }

  /**
   * operator-- (postfix)
   *
   * 
   *
   * @return Updated iterator
   */
  bitmap_array_iterator operator--(int) {
    bitmap_array_iterator it = *this;
    --(*this);
    return it;
  }

  /**
   * operator+=
   *
   * @param i The difference value
   *
   * @return This iterator advanced by the specified amount
   */
  bitmap_array_iterator& operator+=(difference_type i) {
    pos_ += i;
    return *this;
  }

  /**
   * operator-=
   *
   * @param i The difference value
   *
   * @return Returns this iterator with the modified position
   */
  bitmap_array_iterator& operator-=(difference_type i) {
    pos_ -= i;
    return *this;
  }

  /**
   * operator=
   *
   * @param it Another iterator
   *
   * @return This bitmap iterator having the contents of the other
   * bitmap iterator
   */
  bitmap_array_iterator& operator=(const bitmap_array_iterator& it) {
    if (this != &it) {
      array_ = it.array_;
      pos_ = it.pos_;
    }
    return *this;
  }

  /**
   * operator+
   *
   * @param i The difference value
   *
   * @return The advanced iterator
   */
  bitmap_array_iterator operator+(difference_type i) const {
    bitmap_array_iterator it = *this;
    return it += i;
  }

  /**
   * operator-
   *
   * @param i The difference value
   *
   * @return The decremented iterator
   */
  bitmap_array_iterator operator-(difference_type i) const {
    bitmap_array_iterator it = *this;
    return it -= i;
  }

  /**
   * operator[]
   *
   * @param i The index
   *
   * @return reference to value
   */
  reference operator[](difference_type i) const {
    return *(*this + i);
  }

  /**
   * operator==
   *
   * @param it Another iterator
   *
   * @return True if iterator equals other iterator, false otherwise
   */
  bool operator==(const bitmap_array_iterator& it) const {
    return it.pos_ == pos_;
  }

  /**
   * operator!=
   *
   * @param it Another iterator
   *
   * @return True if the iterator does not equal other iterator, false otherwise
   */
  bool operator!=(const bitmap_array_iterator& it) const {
    return !(*this == it);
  }

  /**
   * operator<
   *
   * @param it Another iterator
   *
   * @return True if iterator is less than other iterator, false otherwise
   */
  bool operator<(const bitmap_array_iterator& it) const {
    return pos_ < it.pos_;
  }

  /**
   * operator>
   *
   * @param it Another iterator
   *
   * @return True if iterator is greater than other iterator, false otherwise
   */
  bool operator>(const bitmap_array_iterator& it) const {
    return pos_ > it.pos_;
  }

  /**
   * operator>=
   *
   * @param it Another iterator
   *
   * @return True if iterator is greater than or equals other iterator,
   * false otherwise
   */
  bool operator>=(const bitmap_array_iterator& it) const {
    return !(*this < it);
  }

  /**
   * operator<=
   *
   * @param it Another iterator
   *
   * @return True if iterator is less than or equals other iterator,
   * false otherwise
   */
  bool operator<=(const bitmap_array_iterator& it) const {
    return !(*this > it);
  }

  /**
   * operator-
   *
   * @param it Another iterator
   *
   * @return Distance between this iterator and the other iterator
   */
  difference_type operator-(const bitmap_array_iterator& it) {
    return pos_ - it.pos_;
  }

 private:
  bitmap_array_impl *array_;
  pos_type pos_;
};

/**
 * An iterator for a bitmap array
 */
template<typename bitmap_array_impl>
class const_bitmap_array_iterator {
 public:
  /** The position type */
  typedef typename bitmap_array_impl::pos_type pos_type;

  /** The difference type */
  typedef typename bitmap_array_impl::difference_type difference_type;
  /** The value type */
  typedef typename bitmap_array_impl::value_type value_type;
  /** The bitmap pointer */
  typedef typename bitmap_array_impl::pointer pointer;
  /** Theb bitmap reference */
  typedef typename bitmap_array_impl::reference reference;
  /** The iterator category */
  typedef typename bitmap_array_impl::iterator_category iterator_category;

  /** The constant reference */
  typedef typename bitmap_array_impl::value_type const_reference;

  /**
   * const_bitmap_array_iterator
   *
   * @param array The array
   * @param pos The pos
   */
  const_bitmap_array_iterator(const bitmap_array_impl* array, pos_type pos) {
    array_ = array;
    pos_ = pos;
  }

  /**
   * operator*
   *
   * @return Reference to iterator value
   */
  const_reference operator*() const {
    return array_->get(pos_);
  }

  /**
   * operator++ (prefix)
   *
   * @return Updated iterator
   */
  const_bitmap_array_iterator& operator++() {
    pos_++;
    return *this;
  }

  /**
   * operator++ (postfix)
   *
   *
   * @return Updated iterator
   */
  const_bitmap_array_iterator operator++(int) {
    const_bitmap_array_iterator it = *this;
    ++(*this);
    return it;
  }

  /**
   * operator-- (prefix)
   *
   * @return Updated iterator
   */
  const_bitmap_array_iterator& operator--() {
    pos_--;
    return *this;
  }

  /**
   * operator-- (postfix)
   *
   *
   * @return Updated iterator
   */
  const_bitmap_array_iterator operator--(int) {
    const_bitmap_array_iterator it = *this;
    --(*this);
    return it;
  }

  /**
   * operator+=
   *
   * @param i The difference value
   *
   * @return Updated iterator
   */
  const_bitmap_array_iterator& operator+=(difference_type i) {
    pos_ += i;
    return *this;
  }

  /**
   * operator-=
   *
   * @param i The difference value
   *
   * @return Updated iterator
   */
  const_bitmap_array_iterator& operator-=(difference_type i) {
    pos_ -= i;
    return *this;
  }

  /**
   * operator+
   *
   * @param i The difference value
   *
   * @return Updated iterator
   */
  const_bitmap_array_iterator operator+(difference_type i) const {
    const_bitmap_array_iterator it = *this;
    return it += i;
  }

  /**
   * operator-
   *
   * @param i The difference value
   *
   * @return Updated iterator
   */
  const_bitmap_array_iterator operator-(difference_type i) const {
    const_bitmap_array_iterator it = *this;
    return it -= i;
  }

  /**
   * operator[]
   *
   * @param i The index value
   *
   * @return Reference to value at given index
   */
  const_reference operator[](difference_type i) const {
    return *(*this + i);
  }

  /**
   * operator==
   *
   * @param it Another iterator
   *
   * @return True if iterator equals the other iterator, false otherwise
   */
  bool operator==(const const_bitmap_array_iterator& it) const {
    return it.pos_ == pos_;
  }

  /**
   * operator!=
   *
   * @param it Another iterator
   *
   * @return True if iterator does not equal the other iterator, false otherwise
   */
  bool operator!=(const const_bitmap_array_iterator& it) const {
    return !(*this == it);
  }

  /**
   * operator<
   *
   * @param it Another iterator
   *
   * @return True if iterator is less than the other iterator, false otherwise
   */
  bool operator<(const const_bitmap_array_iterator& it) const {
    return pos_ < it.pos_;
  }

  /**
   * operator>
   *
   * @param it Another iterator
   *
   * @return True if iterator is greater than the other iterator,
   * false otherwise
   */
  bool operator>(const const_bitmap_array_iterator& it) const {
    return pos_ > it.pos_;
  }

  /**
   * operator>=
   *
   * @param it Another iterator
   *
   * @return True if iterator is greater than or equals the other iterator,
   * false otherwise
   */
  bool operator>=(const const_bitmap_array_iterator& it) const {
    return !(*this < it);
  }

  /**
   * operator<=
   *
   * @param it True if iterator is less than or equals the other iterator,
   * false otherwise
   *
   * @return bool
   */
  bool operator<=(const const_bitmap_array_iterator& it) const {
    return !(*this > it);
  }

  /**
   * operator-
   *
   * @param it Another iterator
   *
   * @return Distance between this iterator and the other iterator
   */
  difference_type operator-(const const_bitmap_array_iterator& it) {
    return pos_ - it.pos_;
  }

 private:
  const bitmap_array_impl* array_;
  pos_type pos_;
};

/**
 * Base bitmap array class. The array itself is pre-allocated based on a
 * a specified number of bits and number of elements to store array elements
 * compactly, and uses bitwise arithmetic to access specific elements
 * efficiently.
 */
template<typename T>
class bitmap_array_base : public bitmap {
 public:
  // Constructors and destructors 
  /**
   * Default constructor
   */
  bitmap_array_base()
      : bitmap() {
    num_elements_ = 0;
    bit_width_ = 0;
  }

  /**
   * Copy constructor.
   *
   * @param array Another base bitmap array.
   */
  bitmap_array_base(const bitmap_array_base& array) {
    data_ = array.data_;
    size_ = array.size_;
    num_elements_ = array.num_elements_;
    bit_width_ = array.bit_width_;
  }

  /**
   * Constructor to initialize bitmap array with specified number of elements
   * and bit-width of each element.
   *
   * @param num_elements The number of elements.
   * @param bit_width The bit-width of each element.
   */
  bitmap_array_base(size_type num_elements, width_type bit_width)
      : bitmap(num_elements * bit_width) {
    num_elements_ = num_elements;
    bit_width_ = bit_width;
  }

  /**
   * Destructor.
   */
  virtual ~bitmap_array_base() {
  }

  // Getters
  /**
   * Get the number of elements.
   *
   * @return Number of elements.
   */
  size_type size() const {
    return num_elements_;
  }

  /**
   * Get the bit width of each element.
   *
   * @return Bit width of each element
   */
  width_type bit_width() const {
    return bit_width_;
  }

  /**
   * Check if the array is empty.
   *
   * @return True if array is empty, false otherwise.
   */
  bool empty() const {
    return num_elements_ == 0;
  }

  // Serialization and De-serialization
  /**
   * Serializes the array to a provided output stream.
   *
   * @param out The provided output stream.
   *
   * @return Number of bytes written to output stream.
   */
  virtual size_type serialize(std::ostream& out) override {
    size_t out_size = 0;

    out.write(reinterpret_cast<const char *>(&this->num_elements_),
              sizeof(size_type));
    out_size += sizeof(size_type);

    out.write(reinterpret_cast<const char *>(&this->bit_width_),
              sizeof(width_type));
    out_size += sizeof(width_type);

    out_size += bitmap::serialize(out);

    return out_size;
  }

  /**
   * Deserialize the array from a provided input stream.
   *
   * @param in The input stream.
   *
   * @return Number of bytes read from input stream.
   */
  virtual size_type deserialize(std::istream& in) override {
    size_t in_size = 0;

    in.read(reinterpret_cast<char *>(&this->num_elements_), sizeof(size_type));
    in_size += sizeof(size_type);

    in.read(reinterpret_cast<char *>(&this->bit_width_), sizeof(width_type));
    in_size += sizeof(width_type);

    in_size += bitmap::deserialize(in);

    return in_size;
  }

 protected:
  // Data members
  /** The number of elements */
  size_type num_elements_;
  /** The bit width */
  width_type bit_width_;
};

/**
 * Unsized bitmap array that does not store number of elements in order to
 * save space; does not provide iterators as a consequence. Access/Modify with
 * care, internal bound checks may not be possible
 */
template<typename T>
class unsized_bitmap_array : public bitmap {
 public:
  // Type definitions
  /** The size type */
  typedef typename bitmap_array_base<T>::size_type size_type;
  /** The width type */
  typedef typename bitmap_array_base<T>::width_type width_type;
  /** The position type */
  typedef typename bitmap_array_base<T>::pos_type pos_type;

  /** The bitmap reference */
  typedef value_reference<unsized_bitmap_array<T>> reference;
  /** The value type */
  typedef T value_type;

  // Constructors and destructors
  /**
   * Default constructor
   */
  unsized_bitmap_array()
      : bitmap() {
    bit_width_ = 0;
  }

  /**
   * Copy constructor
   *
   * @param array Another unsized bitmap array
   */
  unsized_bitmap_array(const unsized_bitmap_array& array) {
    data_ = array.data_;
    size_ = array.size_;
    bit_width_ = array.bit_width_;
  }

  /**
   * Constructor to initialize bitmap array with specified number of elements
   * and specified element bit-width
   *
   * @param num_elements The number of elements
   * @param bit_width The bit-width of each element
   */
  unsized_bitmap_array(size_type num_elements, width_type bit_width)
      : bitmap(num_elements * bit_width) {
    bit_width_ = bit_width;
  }

  /**
   * Constructor to initialize bitmap array with specified array of elements,
   * number of elements, and bit-width of each element
   *
   * @param elements The array of elements.
   * @param num_elements The number of elements to store.
   * @param bit_width The bit-width of each element.
   */
  unsized_bitmap_array(T *elements, size_type num_elements,
                       width_type bit_width)
      : unsized_bitmap_array(num_elements, bit_width) {

    for (uint64_t i = 0; i < num_elements; i++) {
      set(i, elements[i]);
    }
  }

  // Accessors and mutators
  /**
   * Sets the value at specified index.
   *
   * @param i The index to set the value at.
   * @param value The value to set.
   */
  void set(pos_type i, T value) {
    this->set_val_pos(i * this->bit_width_, value, this->bit_width_);
  }

  /**
   * Gets the value at specified index.
   *
   * @param i The index to get the value from.
   *
   * @return The value at the specified index.
   */
  T get(pos_type i) const {
    return this->template get_val_pos<T>(i * this->bit_width_, this->bit_width_);
  }

  // Operators, iterators
  /**
   * operator[]
   *
   * @param i The index to get the value from.
   *
   * @return The value at the specified index.
   */
  const T operator[](const pos_type& i) const {
    return get(i);
  }

  /**
   * operator[] (reference)
   *
   * @param i The index to get the reference from.
   *
   * @return The reference at the specified index.
   */
  reference operator[](const pos_type& i) {
    return reference(this, i);
  }

  // Serialization and De-serialization
  /**
   * Serialize the bitmap array to specified output stream.
   *
   * @param out The output stream.
   *
   * @return The number of bytes written to output stream.
   */
  virtual size_type serialize(std::ostream& out) override {
    size_t out_size = 0;

    out.write(reinterpret_cast<const char *>(&this->bit_width_),
              sizeof(width_type));
    out_size += sizeof(width_type);

    out_size += bitmap::serialize(out);

    return out_size;
  }

  /**
   * Deserialize the bitmap from specified input stream.
   *
   * @param in The input stream.
   *
   * @return The number of bytes read from input stream.
   */
  virtual size_type deserialize(std::istream& in) override {
    size_t in_size = 0;

    in.read(reinterpret_cast<char *>(&this->bit_width_), sizeof(width_type));
    in_size += sizeof(width_type);

    in_size += bitmap::deserialize(in);

    return in_size;
  }
 private:
  // Data members
  width_type bit_width_;
};

/**
 * Unsigned bitmap array class. Inherits bitmap array base and only supports
 * storage of unsigned values.
 */
template<typename T>
class unsigned_bitmap_array : public bitmap_array_base<T> {
 public:
  static_assert(!std::numeric_limits<T>::is_signed,
      "Signed types cannot be used with unsigned_bitmap_array.");

  // Type definitions
  /** The size type */
  typedef typename bitmap_array_base<T>::size_type size_type;
  /** The width type */
  typedef typename bitmap_array_base<T>::width_type width_type;
  /** The position type */
  typedef typename bitmap_array_base<T>::pos_type pos_type;

  /** The difference type */
  typedef ptrdiff_t difference_type;
  /** The value type */
  typedef T value_type;
  /** The bitmap array pointer */
  typedef T* pointer;
  /** The value reference */
  typedef value_reference<unsigned_bitmap_array<T>> reference;
  /** The bitmap array iterator */
  typedef bitmap_array_iterator<unsigned_bitmap_array<T>> iterator;
  /** The constant bitmap array iterator */
  typedef const_bitmap_array_iterator<unsigned_bitmap_array<T>> const_iterator;
  /** The iterator category */
  typedef std::random_access_iterator_tag iterator_category;

  /**
   * Default constructor.
   */
  unsigned_bitmap_array()
  : bitmap_array_base<T>() {
  }

  /**
   * Constructor to initialize unsigned bitmap array with specified number of
   * elements and bit-width of each element.
   *
   * @param num_elements The number of elements.
   * @param bit_width The bit-width of each element.
   */
  unsigned_bitmap_array(size_type num_elements, width_type bit_width)
  : bitmap_array_base<T>(num_elements, bit_width) {
  }

  /**
   * Constructor to initialize bitmap array with specified array of elements,
   * number of elements, and bit-width of each element.
   *
   * @param elements The array of elements.
   * @param num_elements The number of elements to store.
   * @param bit_width The bit-width of each element.
   */
  unsigned_bitmap_array(T *elements, size_type num_elements,
      width_type bit_width)
  : bitmap_array_base<T>(num_elements, bit_width) {

    for (uint64_t i = 0; i < this->num_elements_; i++) {
      set(i, elements[i]);
    }
  }

  /**
   * Destructor.
   */
  virtual ~unsigned_bitmap_array() {
  }

  // Accessors and mutators
  /**
   * Sets the value at the specified index.
   *
   * @param i The index to set the value at.
   * @param value The value to set.
   */
  void set(pos_type i, T value) {
    this->set_val_pos(i * this->bit_width_, value, this->bit_width_);
  }

  /**
   * Gets the value at the specified index.
   *
   * @param i The index to get the value from.
   *
   * @return The value at the specified index.
   */
  T get(pos_type i) const {
    return this->template get_val_pos<T>(i * this->bit_width_, this->bit_width_);
  }

  // Operators, iterators
  /**
   * operator[]
   *
   * @param i The index to get the value from.
   *
   * @return The value at the specified index.
   */
  const T operator[](const pos_type& i) const {
    return get(i);
  }

  /**
   * operator[] (reference)
   *
   * @param i The index to get the reference from.
   *
   * @return The reference at the specified index.
   */
  reference operator[](const pos_type& i) {
    return reference(this, i);
  }

  /**
   * Get the begin iterator.
   *
   * @return The begin iterator.
   */
  iterator begin() {
    return iterator(this, 0);
  }

  /**
   * Get the const begin iterator.
   *
   * @return The const begin iterator.
   */
  const_iterator begin() const {
    return const_iterator(this, 0);
  }

  /**
   * Get the const begin iterator.
   *
   * @return The const begin iterator.
   */
  const_iterator cbegin() const {
    return const_iterator(this, 0);
  }

  /**
   * Get the end iterator.
   *
   * @return The end iterator.
   */
  iterator end() {
    return iterator(this, this->num_elements_);
  }

  /**
   * Get the const end iterator.
   *
   * @return The const end iterator.
   */
  const_iterator end() const {
    return const_iterator(this, this->num_elements_);
  }

  /**
   * Get the const end iterator.
   *
   * @return The const end iterator.
   */
  const_iterator cend() const {
    return const_iterator(this, this->num_elements_);
  }

  /**
   * Swaps an unsigned bitmap array with another.
   *
   * @param other The other unsigned bitmap array.
   */
  void swap(const unsigned_bitmap_array<T>& other) {
    using std::swap;
    swap(this->data_, other.data_);
    swap(this->size_, other.size_);
    swap(this->num_elements_, other.num_elements_);
    swap(this->bit_width_, other.bit_width_);
  }
};

/**
 * Signed bitmap array class. Inherits bitmap array base and only supports
 * storage of signed values.
 */
template<typename T>
class signed_bitmap_array : public bitmap_array_base<T> {
 public:
  static_assert(std::numeric_limits<T>::is_signed,
      "Unsigned types cannot be used with signed_bitmap_array.");

  // Type definitions
  /** The size type */
  typedef typename bitmap_array_base<T>::size_type size_type;
  /** The width type */
  typedef typename bitmap_array_base<T>::width_type width_type;
  /** The position type */
  typedef typename bitmap_array_base<T>::pos_type pos_type;

  /** The difference type */
  typedef ptrdiff_t difference_type;
  /** The value type */
  typedef T value_type;
  /** The bitmap array pointer */
  typedef T* pointer;
  /** The value reference */
  typedef value_reference<signed_bitmap_array<T>> reference;
  /** The bitmap array iterator */
  typedef bitmap_array_iterator<signed_bitmap_array<T>> iterator;
  /** The constant bitmap array iterator */
  typedef const_bitmap_array_iterator<signed_bitmap_array<T>> const_iterator;
  /** The iterator category */
  typedef std::random_access_iterator_tag iterator_category;

  /**
   * Default constructor
   */
  signed_bitmap_array()
  : bitmap_array_base<T>() {
  }

  /**
   * Constructor to initialize signed bitmap array with specified number of
   * elements and bit-width of each element.
   *
   * @param num_elements The number of elements.
   * @param bit_width The bit-width of each element.
   */
  signed_bitmap_array(size_type num_elements, width_type bit_width)
  : bitmap_array_base<T>(num_elements, bit_width + 1) {
  }

  /**
   * Constructor to initialize signed bitmap array with specified array of
   * elements, number of elements, and bit-width of each element.
   *
   * @param elements The array of elements.
   * @param num_elements The number of elements to store.
   * @param bit_width The bit-width of each element.
   */
  signed_bitmap_array(T *elements, size_type num_elements, width_type bit_width)
  : bitmap_array_base<T>(num_elements, bit_width + 1) {
    for (uint64_t i = 0; i < this->num_elements_; i++) {
      set(i, elements[i]);
    }
  }

  /**
   * Destructor.
   */
  virtual ~signed_bitmap_array() {
  }

  // Accessors and mutators
  /**
   * Sets the value at the specified index.
   *
   * @param i The index to set the value at.
   * @param value The value to set.
   */
  void set(pos_type i, T value) {
    if (value < 0) {
      this->set_val_pos(i * this->bit_width_, ((-value) << 1) | 1,
          this->bit_width_);
    } else {
      this->set_val_pos(i * this->bit_width_, value << 1, this->bit_width_);
    }
  }

  /**
   * Gets the value at the specified index.
   *
   * @param i The index to get the value from.
   *
   * @return The value at the specified index.
   */
  T get(pos_type i) const {
    T value = this->template get_val_pos<T>(i * this->bit_width_,
        this->bit_width_);
    bool negate = (value & 1);
    return ((value >> 1) ^ -negate) + negate;
    // return (value & 1) ? -(value >> 1) : (value >> 1);
  }

  // Operators, iterators
  /**
   * operator[]
   *
   * @param i The index to get the value from.
   *
   * @return The value at the specified index.
   */
  const T operator[](const pos_type& i) const {
    return get(i);
  }

  /**
   * operator[] (reference)
   *
   * @param i The index to get the reference from.
   *
   * @return The reference at the specified index.
   */
  reference operator[](const pos_type& i) {
    return reference(this, i);
  }

  /**
   * Get the begin iterator.
   *
   * @return The begin iterator.
   */
  iterator begin() {
    return iterator(this, 0);
  }

  /**
   * Get the const begin iterator.
   *
   * @return The const begin iterator.
   */
  const_iterator begin() const {
    return const_iterator(this, 0);
  }

  /**
   * Get the const begin iterator.
   *
   * @return The const begin iterator.
   */
  const_iterator cbegin() const {
    return const_iterator(this, 0);
  }

  /**
   * Get the end iterator.
   *
   * @return The end iterator.
   */
  iterator end() {
    return iterator(this, this->num_elements_);
  }

  /**
   * Get the const end iterator.
   *
   * @return The const end iterator.
   */
  const_iterator end() const {
    return const_iterator(this, this->num_elements_);
  }

  /**
   * Get the const end iterator.
   *
   * @return The const end iterator.
   */
  const_iterator cend() const {
    return const_iterator(this, this->num_elements_);
  }

  /**
   * Swaps an unsigned bitmap array with another.
   *
   * @param other The other unsigned bitmap array.
   */
  void swap(const unsigned_bitmap_array<T>& other) {
    using std::swap;
    swap(this->data_, other.data_);
    swap(this->size_, other.size_);
    swap(this->num_elements_, other.num_elements_);
    swap(this->bit_width_, other.bit_width_);
    swap(this->signs_, other.signs_);
  }
};

}

#endif /* CONFLUO_CONTAINER_BITMAP_BITMAP_ARRAY_H_ */

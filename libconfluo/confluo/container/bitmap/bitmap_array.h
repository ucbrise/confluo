#ifndef CONFLUO_CONTAINER_BITMAP_BITMAP_ARRAY_H_
#define CONFLUO_CONTAINER_BITMAP_BITMAP_ARRAY_H_

#include <limits>

#include "bitmap.h"

namespace confluo {

/** 
 * Reference to a value
 */
template<typename bitmap_array_impl>
class value_reference {
 public:
  typedef typename bitmap_array_impl::pos_type pos_type;
  typedef typename bitmap_array_impl::value_type value_type;
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
   * @param int The next reference to get
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
   * @param int Where to decrement the reference
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
  typedef typename bitmap_array_impl::pos_type pos_type;

  typedef typename bitmap_array_impl::difference_type difference_type;
  typedef typename bitmap_array_impl::value_type value_type;
  typedef typename bitmap_array_impl::pointer pointer;
  typedef typename bitmap_array_impl::reference reference;
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
   * Dereferences the array at the position
   *
   * @return Reference at the given position
   */
  reference operator*() const {
    return reference(array_, pos_);
  }

  /**
   * Advances the iterator by one
   *
   * @return This advanced iterator
   */
  bitmap_array_iterator& operator++() {
    pos_++;
    return *this;
  }

  /**
   * Increments the value at this position
   *
   * @param int The specified integer
   *
   * @return The incremented iterator
   */
  bitmap_array_iterator operator++(int) {
    bitmap_array_iterator it = *this;
    ++(*this);
    return it;
  }

  /**
   * Decrements the position of the iterator
   *
   * @return This iterator
   */
  bitmap_array_iterator& operator--() {
    pos_--;
    return *this;
  }

  /**
   * Decrements the value at the position
   *
   * @param int The specified integer
   *
   * @return This iterator with the decremented value
   */
  bitmap_array_iterator operator--(int) {
    bitmap_array_iterator it = *this;
    --(*this);
    return it;
  }

  /**
   * Increments the position of the iterator by a specified amount
   *
   * @param i The amount to increment the position by
   *
   * @return This iterator advanced by the specified amount
   */
  bitmap_array_iterator& operator+=(difference_type i) {
    pos_ += i;
    return *this;
  }

  /**
   * Decrements the iterator position by the specified amount
   *
   * @param i The difference
   *
   * @return Returns this iterator with the modified position
   */
  bitmap_array_iterator& operator-=(difference_type i) {
    pos_ -= i;
    return *this;
  }

  /**
   * Assigns another bitmap iterator to this bitmap iterator
   *
   * @param it The other iterator to assign to this iterator
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
   * Advances the position of the iterator by a specified amount
   *
   * @param i The difference
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
   * @param i The i
   *
   * @return bitmap_array_iterator
   */
  bitmap_array_iterator operator-(difference_type i) const {
    bitmap_array_iterator it = *this;
    return it -= i;
  }

  /**
   * operator[]
   *
   * @param i The i
   *
   * @return reference
   */
  reference operator[](difference_type i) const {
    return *(*this + i);
  }

  /**
   * operator==
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator==(const bitmap_array_iterator& it) const {
    return it.pos_ == pos_;
  }

  /**
   * operator!=
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator!=(const bitmap_array_iterator& it) const {
    return !(*this == it);
  }

  /**
   * operator<
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator<(const bitmap_array_iterator& it) const {
    return pos_ < it.pos_;
  }

  /**
   * operator>
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator>(const bitmap_array_iterator& it) const {
    return pos_ > it.pos_;
  }

  /**
   * operator>=
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator>=(const bitmap_array_iterator& it) const {
    return !(*this < it);
  }

  /**
   * operator<=
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator<=(const bitmap_array_iterator& it) const {
    return !(*this > it);
  }

  /**
   * operator-
   *
   * @param it The it
   *
   * @return difference_type
   */
  difference_type operator-(const bitmap_array_iterator& it) {
    return pos_ - it.pos_;
  }

 private:
  bitmap_array_impl *array_;
  pos_type pos_;
};

template<typename bitmap_array_impl>
class const_bitmap_array_iterator {
 public:
  typedef typename bitmap_array_impl::pos_type pos_type;

  typedef typename bitmap_array_impl::difference_type difference_type;
  typedef typename bitmap_array_impl::value_type value_type;
  typedef typename bitmap_array_impl::pointer pointer;
  typedef typename bitmap_array_impl::reference reference;
  typedef typename bitmap_array_impl::iterator_category iterator_category;

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
   * operator
   *
   * @return const_reference
   */
  const_reference operator*() const {
    return array_->get(pos_);
  }

  /**
   * operator++
   *
   * @return const_bitmap_array_iterator&
   */
  const_bitmap_array_iterator& operator++() {
    pos_++;
    return *this;
  }

  /**
   * operator++
   *
   * @param int The int
   *
   * @return const_bitmap_array_iterator
   */
  const_bitmap_array_iterator operator++(int) {
    const_bitmap_array_iterator it = *this;
    ++(*this);
    return it;
  }

  /**
   * operator--
   *
   * @return const_bitmap_array_iterator&
   */
  const_bitmap_array_iterator& operator--() {
    pos_--;
    return *this;
  }

  /**
   * operator--
   *
   * @param int The int
   *
   * @return const_bitmap_array_iterator
   */
  const_bitmap_array_iterator operator--(int) {
    const_bitmap_array_iterator it = *this;
    --(*this);
    return it;
  }

  /**
   * operator+=
   *
   * @param i The i
   *
   * @return const_bitmap_array_iterator&
   */
  const_bitmap_array_iterator& operator+=(difference_type i) {
    pos_ += i;
    return *this;
  }

  /**
   * operator-=
   *
   * @param i The i
   *
   * @return const_bitmap_array_iterator&
   */
  const_bitmap_array_iterator& operator-=(difference_type i) {
    pos_ -= i;
    return *this;
  }

  /**
   * operator+
   *
   * @param i The i
   *
   * @return const_bitmap_array_iterator
   */
  const_bitmap_array_iterator operator+(difference_type i) const {
    const_bitmap_array_iterator it = *this;
    return it += i;
  }

  /**
   * operator-
   *
   * @param i The i
   *
   * @return const_bitmap_array_iterator
   */
  const_bitmap_array_iterator operator-(difference_type i) const {
    const_bitmap_array_iterator it = *this;
    return it -= i;
  }

  /**
   * operator[]
   *
   * @param i The i
   *
   * @return const_reference
   */
  const_reference operator[](difference_type i) const {
    return *(*this + i);
  }

  /**
   * operator==
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator==(const const_bitmap_array_iterator& it) const {
    return it.pos_ == pos_;
  }

  /**
   * operator!=
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator!=(const const_bitmap_array_iterator& it) const {
    return !(*this == it);
  }

  /**
   * operator<
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator<(const const_bitmap_array_iterator& it) const {
    return pos_ < it.pos_;
  }

  /**
   * operator>
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator>(const const_bitmap_array_iterator& it) const {
    return pos_ > it.pos_;
  }

  /**
   * operator>=
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator>=(const const_bitmap_array_iterator& it) const {
    return !(*this < it);
  }

  /**
   * operator<=
   *
   * @param it The it
   *
   * @return bool
   */
  bool operator<=(const const_bitmap_array_iterator& it) const {
    return !(*this > it);
  }

  /**
   * operator-
   *
   * @param it The it
   *
   * @return difference_type
   */
  difference_type operator-(const const_bitmap_array_iterator& it) {
    return pos_ - it.pos_;
  }

 private:
  const bitmap_array_impl* array_;
  pos_type pos_;
};

template<typename T>
class bitmap_array_base : public bitmap {
 public:
  // Constructors and destructors 
  bitmap_array_base()
      : bitmap() {
    num_elements_ = 0;
    bit_width_ = 0;
  }

  /**
   * bitmap_array_base
   *
   * @param array The array
   */
  bitmap_array_base(const bitmap_array_base& array) {
    data_ = array.data_;
    size_ = array.size_;
    num_elements_ = array.num_elements_;
    bit_width_ = array.bit_width_;
  }

  /**
   * bitmap_array_base
   *
   * @param num_elements The num_elements
   * @param bit_width The bit_width
   */
  bitmap_array_base(size_type num_elements, width_type bit_width)
      : bitmap(num_elements * bit_width) {
    num_elements_ = num_elements;
    bit_width_ = bit_width;
  }

  /**
   * ~bitmap_array_base
   */
  virtual ~bitmap_array_base() {
  }

  // Getters
  /**
   * size
   *
   * @return size_type
   */
  size_type size() const {
    return num_elements_;
  }

  /**
   * bit_width
   *
   * @return width_type
   */
  width_type bit_width() const {
    return bit_width_;
  }

  /**
   * empty
   *
   * @return bool
   */
  bool empty() const {
    return num_elements_ == 0;
  }

  // Serialization and De-serialization
  /**
   * serialize
   *
   * @param out The out
   *
   * @return size_type
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
   * deserialize
   *
   * @param in The in
   *
   * @return size_type
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
  size_type num_elements_;
  width_type bit_width_;
};

// Unsigned bitmap array that does not store number of elements in order to
// save space; does not provide iterators as a consequence. Access/Modify with
// care, internal bound checks may not be possible
template<typename T>
class unsized_bitmap_array : public bitmap {
 public:
  // Type definitions
  typedef typename bitmap_array_base<T>::size_type size_type;
  typedef typename bitmap_array_base<T>::width_type width_type;
  typedef typename bitmap_array_base<T>::pos_type pos_type;

  typedef value_reference<unsized_bitmap_array<T>> reference;
  typedef T value_type;

  // Constructors and destructors
  /**
   * unsized_bitmap_array
   */
  unsized_bitmap_array()
      : bitmap() {
    bit_width_ = 0;
  }

  /**
   * unsized_bitmap_array
   *
   * @param array The array
   */
  unsized_bitmap_array(const unsized_bitmap_array& array) {
    data_ = array.data_;
    size_ = array.size_;
    bit_width_ = array.bit_width_;
  }

  /**
   * unsized_bitmap_array
   *
   * @param num_elements The num_elements
   * @param bit_width The bit_width
   */
  unsized_bitmap_array(size_type num_elements, width_type bit_width)
      : bitmap(num_elements * bit_width) {
    bit_width_ = bit_width;
  }

  /**
   * unsized_bitmap_array
   *
   * @param elements The elements
   * @param num_elements The num_elements
   * @param bit_width The bit_width
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
   * set
   *
   * @param i The i
   * @param value The value
   */
  void set(pos_type i, T value) {
    this->set_val_pos(i * this->bit_width_, value, this->bit_width_);
  }

  /**
   * get
   *
   * @param i The i
   *
   * @return T
   */
  T get(pos_type i) const {
    return this->template get_val_pos<T>(i * this->bit_width_, this->bit_width_);
  }

  // Operators, iterators
  /**
   * operator[]
   *
   * @param i The i
   *
   * @return T
   */
  const T operator[](const pos_type& i) const {
    return get(i);
  }

  /**
   * operator[]
   *
   * @param i The i
   *
   * @return reference
   */
  reference operator[](const pos_type& i) {
    return reference(this, i);
  }

  // Serialization and De-serialization
  /**
   * serialize
   *
   * @param out The out
   *
   * @return size_type
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
   * deserialize
   *
   * @param in The in
   *
   * @return size_type
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

template<typename T>
class unsigned_bitmap_array : public bitmap_array_base<T> {
 public:
  static_assert(!std::numeric_limits<T>::is_signed,
      "Signed types cannot be used with unsigned_bitmap_array.");

  // Type definitions
  typedef typename bitmap_array_base<T>::size_type size_type;
  typedef typename bitmap_array_base<T>::width_type width_type;
  typedef typename bitmap_array_base<T>::pos_type pos_type;

  typedef ptrdiff_t difference_type;
  typedef T value_type;
  typedef T* pointer;
  typedef value_reference<unsigned_bitmap_array<T>> reference;
  typedef bitmap_array_iterator<unsigned_bitmap_array<T>> iterator;
  typedef const_bitmap_array_iterator<unsigned_bitmap_array<T>> const_iterator;
  typedef std::random_access_iterator_tag iterator_category;

  /**
   * unsigned_bitmap_array
   */
  unsigned_bitmap_array()
      : bitmap_array_base<T>() {
  }

  /**
   * unsigned_bitmap_array
   *
   * @param num_elements The num_elements
   * @param bit_width The bit_width
   */
  unsigned_bitmap_array(size_type num_elements, width_type bit_width)
      : bitmap_array_base<T>(num_elements, bit_width) {
  }

  /**
   * unsigned_bitmap_array
   *
   * @param elements The elements
   * @param num_elements The num_elements
   * @param bit_width The bit_width
   */
  unsigned_bitmap_array(T *elements, size_type num_elements,
                        width_type bit_width)
      : bitmap_array_base<T>(num_elements, bit_width) {

    for (uint64_t i = 0; i < this->num_elements_; i++) {
      set(i, elements[i]);
    }
  }

  /**
   * ~unsigned_bitmap_array
   */
  virtual ~unsigned_bitmap_array() {
  }

  // Accessors and mutators
  /**
   * set
   *
   * @param i The i
   * @param value The value
   */
  void set(pos_type i, T value) {
    this->set_val_pos(i * this->bit_width_, value, this->bit_width_);
  }

  /**
   * get
   *
   * @param i The i
   *
   * @return T
   */
  T get(pos_type i) const {
    return this->template get_val_pos<T>(i * this->bit_width_, this->bit_width_);
  }

  // Operators, iterators
  /**
   * operator[]
   *
   * @param i The i
   *
   * @return T
   */
  const T operator[](const pos_type& i) const {
    return get(i);
  }

  /**
   * operator[]
   *
   * @param i The i
   *
   * @return reference
   */
  reference operator[](const pos_type& i) {
    return reference(this, i);
  }

  /**
   * begin
   *
   * @return iterator
   */
  iterator begin() {
    return iterator(this, 0);
  }

  /**
   * begin
   *
   * @return const_iterator
   */
  const_iterator begin() const {
    return const_iterator(this, 0);
  }

  /**
   * cbegin
   *
   * @return const_iterator
   */
  const_iterator cbegin() const {
    return const_iterator(this, 0);
  }

  /**
   * end
   *
   * @return iterator
   */
  iterator end() {
    return iterator(this, this->num_elements_);
  }

  /**
   * end
   *
   * @return const_iterator
   */
  const_iterator end() const {
    return const_iterator(this, this->num_elements_);
  }

  /**
   * cend
   *
   * @return const_iterator
   */
  const_iterator cend() const {
    return const_iterator(this, this->num_elements_);
  }

  /**
   * swap
   *
   * @param other The other
   */
  void swap(const unsigned_bitmap_array<T>& other) {
    using std::swap;
    swap(this->data_, other.data_);
    swap(this->size_, other.size_);
    swap(this->num_elements_, other.num_elements_);
    swap(this->bit_width_, other.bit_width_);
  }
};

template<typename T>
class signed_bitmap_array : public bitmap_array_base<T> {
 public:
  static_assert(std::numeric_limits<T>::is_signed,
      "Unsigned types cannot be used with signed_bitmap_array.");

  // Type definitions
  typedef typename bitmap_array_base<T>::size_type size_type;
  typedef typename bitmap_array_base<T>::width_type width_type;
  typedef typename bitmap_array_base<T>::pos_type pos_type;

  typedef ptrdiff_t difference_type;
  typedef T value_type;
  typedef T* pointer;
  typedef value_reference<signed_bitmap_array<T>> reference;
  typedef bitmap_array_iterator<signed_bitmap_array<T>> iterator;
  typedef const_bitmap_array_iterator<signed_bitmap_array<T>> const_iterator;
  typedef std::random_access_iterator_tag iterator_category;

  /**
   * signed_bitmap_array
   */
  signed_bitmap_array()
      : bitmap_array_base<T>() {
  }

  /**
   * signed_bitmap_array
   *
   * @param num_elements The num_elements
   * @param bit_width The bit_width
   */
  signed_bitmap_array(size_type num_elements, width_type bit_width)
      : bitmap_array_base<T>(num_elements, bit_width + 1) {
  }

  /**
   * signed_bitmap_array
   *
   * @param elements The elements
   * @param num_elements The num_elements
   * @param bit_width The bit_width
   */
  signed_bitmap_array(T *elements, size_type num_elements, width_type bit_width)
      : bitmap_array_base<T>(num_elements, bit_width + 1) {
    for (uint64_t i = 0; i < this->num_elements_; i++) {
      set(i, elements[i]);
    }
  }

  /**
   * ~signed_bitmap_array
   */
  virtual ~signed_bitmap_array() {
  }

  // Accessors and mutators
  /**
   * set
   *
   * @param i The i
   * @param value The value
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
   * get
   *
   * @param i The i
   *
   * @return T
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
   * @param i The i
   *
   * @return T
   */
  const T operator[](const pos_type& i) const {
    return get(i);
  }

  /**
   * operator[]
   *
   * @param i The i
   *
   * @return reference
   */
  reference operator[](const pos_type& i) {
    return reference(this, i);
  }

  /**
   * begin
   *
   * @return iterator
   */
  iterator begin() {
    return iterator(this, 0);
  }

  /**
   * begin
   *
   * @return const_iterator
   */
  const_iterator begin() const {
    return const_iterator(this, 0);
  }

  /**
   * cbegin
   *
   * @return const_iterator
   */
  const_iterator cbegin() const {
    return const_iterator(this, 0);
  }

  /**
   * end
   *
   * @return iterator
   */
  iterator end() {
    return iterator(this, this->num_elements_);
  }

  /**
   * end
   *
   * @return const_iterator
   */
  const_iterator end() const {
    return iterator(this, this->num_elements_);
  }

  /**
   * cend
   *
   * @return const_iterator
   */
  const_iterator cend() const {
    return iterator(this, this->num_elements_);
  }

  /**
   * swap
   *
   * @param other The other
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

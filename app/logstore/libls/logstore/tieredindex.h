#ifndef TIEREDINDEX_H_
#define TIEREDINDEX_H_

#include <atomic>
#include <array>

#include "entrylist.h"

namespace monolog {

/**
 * @brief The unit of indexing in tiered indexes.
 * @details The basic unit of indexing in tiered indexes. An
 * indexlet stores a fixed-size array with atomic references to
 * objects of value type.
 * 
 * @tparam T Tyepe of the value.
 * @tparam SIZE = 65536 Size of the fixed-size array.
 */
template<typename T, size_t SIZE = 65536>
class indexlet {
 public:
  typedef atomic::type<T*> atomic_ref;

  /**
   * @brief Constructor for the indexlet.
   * @details Constructor for the indexlet. Initializes all array references to
   * null.
   */
  indexlet() {
    for (uint32_t i = 0; i < SIZE; i++) {
      idx_[i].store(NULL, std::memory_order_release);
    }
  }

  /**
   * @brief Virtual destructor for indexlet.
   * @details Virtual destructor for indexlet. Deletes all array references.
   */
  virtual ~indexlet() {
    for (uint32_t i = 0; i < SIZE; i++) {
      delete idx_[i].load(std::memory_order_acquire);
    }
  }

  /**
   * @brief Creates and fetches the value at a given index.
   * @details Obtain the value at a given index; creates required
   * internal structure for the value if it does not exist.
   *
   * @param index The index to lookup.
   * @return Pointer to the value.
   */
  T* get(const uint32_t i) {
    if (idx_[i].load(std::memory_order_acquire) == NULL) {
      T* item = new T();
      T* null_ptr = NULL;

      // Only one thread will be successful in replacing the NULL reference with newly
      // allocated item.
      if (!atomic::strong::cas(&idx_[i], &null_ptr, item)) {
        // All other threads will deallocate the newly allocated item.
        delete item;
      }
    }

    return idx_[i].load(std::memory_order_acquire);
  }

  /**
   * @brief Operator override for accessing the value at a given index.
   * @details Overrides operator for accessing accessing the value at a
   * given index. Creates any required internal structure.
   *
   * @param i Index of the value.
   */
  T* operator[](const uint32_t i) {
    return get(i);
  }

  /**
   * @brief Function for getting the value at a given index.
   * @details Function for getting the value at a given index.
   *
   * @param i The index to lookup.
   * @return Pointer to the value.
   */
  T* at(const uint32_t i) const {
    return idx_[i].load(std::memory_order_acquire);
  }

  /**
   * @brief Get the size of the indexlet.
   * @details Get the size of the indexlet.
   * @return Size of the indexlet.
   */
  size_t size() {
    return SIZE;
  }

  /**
   * @brief Get the storage size in bytes of the indexlet.
   * @details Get the storage size in bytes of the indexlet.
   * @return The storage size in bytes of the indexlet.
   */
  size_t storage_size() {
    size_t tot_size = SIZE * sizeof(atomic_ref);
    for (uint32_t i = 0; i < SIZE; i++) {
      if (idx_[i].load(std::memory_order_acquire) != NULL) {
        tot_size += idx_[i].load(std::memory_order_acquire)->storage_size();
      }
    }
    return tot_size;
  }

 private:
  std::array<atomic_ref, SIZE> idx_;
};

/**
 * @brief Base class for tiered indexes.
 * @details This is the base class for tiered indexes,
 * and exposes a pure virtual at() function for getting
 * the index value for a certain key.
 *
 * @tparam value_type = entry_list The value type for the index.
 */
template<typename value_type = entry_list>
class __tiered_index_base {
 public:
  /**
   * @brief Virtual destructor for __tiered_index_base.
   * @details Virtual destructor for __tiered_index_base.
   */
  virtual ~__tiered_index_base() {
  }

  /**
   * @brief Pure virtual function for getting the value corresponding to a key.
   * @details Pure virtual function for getting the value corresponding to a
   * key.
   *
   * @param key The key to lookup.
   * @return Pointer to the value.
   */
  virtual value_type* at(const uint64_t key) const = 0;
};

/**
 * @brief Tiered index of depth 1.
 * @details Tiered index of depth 1.
 *
 * @tparam SIZE Maximum number of contiguous key entries.
 * @tparam value_type = entry_list The value type for the index.
 */
template<size_t SIZE, typename value_type = entry_list>
class __index_depth1 : public __tiered_index_base<value_type> {
 public:
  /**
   * @brief Virtual destructor for __index_depth1.
   * @details Virtual destructor for __index_depth1.
   */
  virtual ~__index_depth1() {
  }

  /**
   * @brief Creates and fetches the value corresponding to the key.
   * @details Obtain the value corresponding to the key; creates required
   * internal structure for the value if it does not exist.
   *
   * @param key The key to lookup.
   * @return Pointer to the value.
   */
  value_type* get(const uint64_t key) {
    return idx_[key];
  }

  /**
   * @brief Operator override for accessing the indexlet at a given index.
   * @details Overrides operator for accessing accessing the indexlet at a
   * given index. Creates any required internal structure.
   *
   * @param i Index of the indexlet.
   */
  value_type* operator[](const uint64_t i) {
    return idx_[i];
  }

  /**
   * @brief Function for getting the value corresponding to a key.
   * @details Function for getting the value corresponding to a key.
   *
   * @param key The key to lookup.
   * @return Pointer to the value.
   */
  value_type* at(const uint64_t key) const override {
    return idx_.at(key);
  }

  /**
   * @brief Add a new (key, value-entry) pair to the index.
   * @details Add a new (key, value-entry) pair to the index.
   *
   * @param key The key to add.
   * @param val The value-entry to add.
   */
  void add_entry(const uint64_t key, const uint64_t val) {
    value_type* list = get(key);
    list->push_back(val);
  }

  /**
   * @brief Get the maximum possible size (in number of keys) for the index.
   * @details Get the maximum possible size (in number of keys) for the index.
   * @return The maximum possible size (in number of keys) for the index.
   */
  size_t max_size() {
    return SIZE;
  }

  /**
   * @brief Get the storage size in bytes of the index.
   * @details Get the storage size in bytes of the index.
   * @return The storage size in bytes of the index.
   */
  size_t storage_size() {
    return idx_.storage_size();
  }

 protected:
  indexlet<value_type, SIZE> idx_;
};

/**
 * @brief Tiered index of depth 2.
 * @details Tiered index of depth 2.
 *
 * @tparam SIZE1 Maximum number of contiguous key entries in level 1 indexlet.
 * @tparam SIZE2 Maximum number of contiguous key entries in level 2 indexlet.
 * @tparam value_type = entry_list The value type for the index.
 */
template<size_t SIZE1, size_t SIZE2, typename value_type = entry_list>
class __index_depth2 : public __tiered_index_base<value_type> {
 public:
  /**
   * @brief Virtual destructor for __index_depth2.
   * @details Virtual destructor for __index_depth2.
   */
  virtual ~__index_depth2() {
  }

  /**
   * @brief Creates and fetches the value corresponding to the key.
   * @details Obtain the value corresponding to the key; creates required
   * internal structure for the value if it does not exist.
   *
   * @param key The key to lookup.
   * @return Pointer to the value.
   */
  value_type* get(const uint64_t key) {
    __index_depth1 <SIZE2, value_type>* ilet = idx_[key / SIZE2];
    return ilet->get(key % SIZE2);
  }

  /**
   * @brief Operator override for accessing the indexlet at a given index.
   * @details Overrides operator for accessing accessing the indexlet at a
   * given index. Creates any required internal structure.
   *
   * @param i Index of the indexlet.
   */
  __index_depth1 <SIZE2, value_type>* operator[](const uint64_t i) {
    return idx_[i];
  }

  /**
   * @brief Function for getting the value corresponding to a key.
   * @details Function for getting the value corresponding to a key.
   *
   * @param key The key to lookup.
   * @return Pointer to the value.
   */
  value_type* at(const uint64_t key) const override {
    __index_depth1 <SIZE2, value_type>* ilet = idx_.at(key / SIZE2);
    if (ilet)
      return ilet->at(key % SIZE2);
    return NULL;
  }

  /**
   * @brief Add a new (key, value-entry) pair to the index.
   * @details Add a new (key, value-entry) pair to the index.
   *
   * @param key The key to add.
   * @param val The value-entry to add.
   */
  void add_entry(const uint64_t key, const uint64_t val) {
    value_type* list = get(key);
    list->push_back(val);
  }

  /**
   * @brief Get the maximum possible size (in number of keys) for the index.
   * @details Get the maximum possible size (in number of keys) for the index.
   * @return The maximum possible size (in number of keys) for the index.
   */
  size_t max_size() {
    return SIZE1 * SIZE2;
  }

  /**
   * @brief Get the storage size in bytes of the index.
   * @details Get the storage size in bytes of the index.
   * @return The storage size in bytes of the index.
   */
  size_t storage_size() {
    return idx_.storage_size();
  }

 private:
  indexlet<__index_depth1 <SIZE2, value_type>, SIZE1> idx_;
};

/**
 * @brief Tiered index of depth 3.
 * @details Tiered index of depth 3.
 *
 * @tparam SIZE1 Maximum number of contiguous key entries in level 1 indexlet.
 * @tparam SIZE2 Maximum number of contiguous key entries in level 2 indexlet.
 * @tparam SIZE3 Maximum number of contiguous key entries in level 3 indexlet.
 * @tparam value_type = entry_list The value type for the index.
 */
template<size_t SIZE1, size_t SIZE2, size_t SIZE3,
    typename value_type = entry_list>
class __index_depth3 : public __tiered_index_base<value_type> {
 public:
  /**
   * @brief Virtual destructor for __index_depth3.
   * @details Virtual destructor for __index_depth3.
   */
  virtual ~__index_depth3() {
  }

  /**
   * @brief Creates and fetches the value corresponding to the key.
   * @details Obtain the value corresponding to the key; creates required
   * internal structure for the value if it does not exist.
   *
   * @param key The key to lookup.
   * @return Pointer to the value.
   */
  value_type* get(const uint64_t key) {
    __index_depth2 <SIZE2, SIZE3, value_type>* ilet =
        idx_[key / (SIZE2 * SIZE3)];
    return ilet->get(key % (SIZE2 * SIZE3));
  }

  /**
   * @brief Operator override for accessing the indexlet at a given index.
   * @details Overrides operator for accessing accessing the indexlet at a
   * given index. Creates any required internal structure.
   *
   * @param i Index of the indexlet.
   */
  __index_depth2 <SIZE2, SIZE3, value_type>* operator[](const uint64_t i) {
    return idx_[i];
  }

  /**
   * @brief Function for getting the value corresponding to a key.
   * @details Function for getting the value corresponding to a key.
   *
   * @param key The key to lookup.
   * @return Pointer to the value.
   */
  value_type* at(const uint64_t key) const override {
    __index_depth2 <SIZE2, SIZE3, value_type>* ilet = idx_.at(
        key / (SIZE2 * SIZE3));
    if (ilet)
      return ilet->at(key % (SIZE2 * SIZE3));
    return NULL;
  }

  /**
   * @brief Add a new (key, value-entry) pair to the index.
   * @details Add a new (key, value-entry) pair to the index.
   *
   * @param key The key to add.
   * @param val The value-entry to add.
   */
  void add_entry(const uint64_t key, const uint64_t val) {
    value_type* list = get(key);
    list->push_back(val);
  }

  /**
   * @brief Get the maximum possible size (in number of keys) for the index.
   * @details Get the maximum possible size (in number of keys) for the index.
   * @return The maximum possible size (in number of keys) for the index.
   */
  size_t max_size() {
    return SIZE1 * SIZE2 * SIZE3;
  }

  /**
   * @brief Get the storage size in bytes of the index.
   * @details Get the storage size in bytes of the index.
   * @return The storage size in bytes of the index.
   */
  size_t storage_size() {
    return idx_.storage_size();
  }

 private:
  indexlet<__index_depth2 <SIZE2, SIZE3, value_type>, SIZE1> idx_;
};

/**
 * @brief Tiered index of depth 4.
 * @details Tiered index of depth 4.
 *
 * @tparam SIZE1 Maximum number of contiguous key entries in level 1 indexlet.
 * @tparam SIZE2 Maximum number of contiguous key entries in level 2 indexlet.
 * @tparam SIZE3 Maximum number of contiguous key entries in level 3 indexlet.
 * @tparam SIZE3 Maximum number of contiguous key entries in level 4 indexlet.
 * @tparam value_type = entry_list The value type for the index.
 */
template<size_t SIZE1, size_t SIZE2, size_t SIZE3, size_t SIZE4,
    typename value_type = entry_list>
class __index_depth4 : public __tiered_index_base<value_type> {
 public:
  /**
   * @brief Virtual destructor for __index_depth4.
   * @details Virtual destructor for __index_depth4.
   */
  virtual ~__index_depth4() {
  }

  /**
   * @brief Creates and fetches the value corresponding to the key.
   * @details Obtain the value corresponding to the key; creates required
   * internal structure for the value if it does not exist.
   *
   * @param key The key to lookup.
   * @return Pointer to the value.
   */
  value_type* get(const uint64_t key) {
    __index_depth3 <SIZE2, SIZE3, SIZE4, value_type>* ilet = idx_[key
        / (SIZE2 * SIZE3 * SIZE4)];
    return ilet->get(key % (SIZE2 * SIZE3 * SIZE4));
  }

  /**
   * @brief Operator override for accessing the indexlet at a given index.
   * @details Overrides operator for accessing accessing the indexlet at a
   * given index. Creates any required internal structure.
   *
   * @param i Index of the indexlet.
   */
  __index_depth3 <SIZE2, SIZE3, SIZE4, value_type>* operator[](
      const uint64_t i) {
    return idx_[i];
  }

  /**
   * @brief Function for getting the value corresponding to a key.
   * @details Function for getting the value corresponding to a key.
   *
   * @param key The key to lookup.
   * @return Pointer to the value.
   */
  value_type* at(const uint64_t key) const override {
    __index_depth3 <SIZE2, SIZE3, SIZE4, value_type>* ilet = idx_.at(
        key / (SIZE2 * SIZE3 * SIZE4));
    if (ilet)
      return ilet->at(key % (SIZE2 * SIZE3 * SIZE4));
    return NULL;
  }

  /**
   * @brief Add a new (key, value-entry) pair to the index.
   * @details Add a new (key, value-entry) pair to the index.
   *
   * @param key The key to add.
   * @param val The value-entry to add.
   */
  void add_entry(const uint64_t key, uint64_t val) {
    value_type* list = get(key);
    list->push_back(val);
  }

  /**
   * @brief Get the maximum possible size (in number of keys) for the index.
   * @details Get the maximum possible size (in number of keys) for the index.
   * @return The maximum possible size (in number of keys) for the index.
   */
  size_t max_size() {
    return SIZE1 * SIZE2 * SIZE3;
  }

  /**
   * @brief Get the storage size in bytes of the index.
   * @details Get the storage size in bytes of the index.
   * @return The storage size in bytes of the index.
   */
  size_t storage_size() {
    return idx_.storage_size();
  }

 private:
  indexlet<__index_depth3 <SIZE2, SIZE3, SIZE4, value_type>, SIZE1> idx_;
};

/**
 * Useful type-definitions.
 */
typedef __tiered_index_base <> tiered_index_base;
typedef __index_depth1 <256> __index1;
typedef __index_depth1 <65536> __index2;
typedef __index_depth2 <65536, 256> __index3;
typedef __index_depth2 <65536, 65536> __index4;
typedef __index_depth3 <65536, 65536, 256> __index5;
typedef __index_depth3 <65536, 65536, 65536> __index6;
typedef __index_depth4 <65536, 65536, 65536, 256> __index7;
typedef __index_depth4 <65536, 65536, 65536, 65536> __index8;

}
#endif /* TIEREDINDEX_H_ */

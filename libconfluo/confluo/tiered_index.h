#ifndef DIALOG_TIERED_INDEX_H_
#define DIALOG_TIERED_INDEX_H_

#include <array>

#include "atomic.h"
#include "monolog.h"
#include "math_utils.h"
#include "assertions.h"

// TODO: Improve documentation

using namespace ::utils;

namespace confluo {

namespace index {

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
    for (uint32_t i = 0; i < SIZE; i++)
      atomic::init(&idx_[i], static_cast<T*>(nullptr));
  }

  /**
   * @brief Virtual destructor for indexlet.
   * @details Virtual destructor for indexlet. Deletes all array references.
   */
  virtual ~indexlet() {
    for (uint32_t i = 0; i < SIZE; i++)
      delete atomic::load(&idx_[i]);
  }

  /**
   * @brief Creates and fetches the value at a given index.
   * @details Obtain the value at a given index; creates required
   * internal structure for the value if it does not exist.
   *
   * @param i The index to lookup.
   * @return Pointer to the value.
   */
  T* get_or_create(const uint32_t i) {
    T* item;
    if ((item = atomic::load(&idx_[i])) == nullptr) {
      item = new T();
      T* current_item = nullptr;

      // Only one thread will be successful in replacing the NULL reference with newly
      // allocated item.
      if (!atomic::strong::cas(&idx_[i], &current_item, item)) {
        // All other threads will deallocate the newly allocated item.
        delete item;
        return current_item;
      }
    }
    return item;
  }

  /**
   * @brief Fetches the atomic pointer at a given index.
   * @details Obtain the atomic pointer at a given index.
   *
   * @param i The index to lookup.
   * @return Atomic pointer to the value.
   */
  atomic_ref* get_atomic(const uint32_t i) {
    return &idx_[i];
  }

  /**
   * @brief Operator override for accessing the value at a given index.
   * @details Overrides operator for accessing accessing the value at a
   * given index. Creates any required internal structure.
   *
   * @param i Index of the value.
   */
  T* operator[](const uint32_t i) {
    return get_or_create(i);
  }

  /**
   * @brief Function for getting the value at a given index.
   * @details Function for getting the value at a given index.
   *
   * @param i The index to lookup.
   * @return Pointer to the value.
   */
  T* get(const uint32_t i) const {
    return atomic::load(&idx_[i]);
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
    T* ptr;
    for (uint32_t i = 0; i < SIZE; i++)
      if ((ptr = atomic::load(&idx_[i])) != nullptr)
        tot_size += ptr->storage_size();
    return tot_size;
  }

 private:
  std::array<atomic_ref, SIZE> idx_;
};

struct empty_stats {
  empty_stats(const empty_stats& other) {
  }
};

template<typename T, size_t K, size_t D, typename stats = empty_stats>
class tiered_index {
 public:
  typedef tiered_index<T, K, D - 1, stats> child_type;
  typedef indexlet<child_type, K> idx_type;

  static uint64_t CHILD_BLOCK;

  /**
   * @brief Constructor for the tiered index
   */
  tiered_index() {
    atomic::init(&stats_, static_cast<stats*>(nullptr));
  }

  /**
   * @brief Access the child data from the key
   * @param key The key for lookup
   * @return The pointer to the child
   */
  T* operator[](const uint64_t key) {
    child_type* c = get_or_create_child(key / CHILD_BLOCK);
    return (*c)[key % CHILD_BLOCK];
  }

  /**
   * Access the child data passing in the update arguments
   * @param key The key for lookup
   * @param u Reference to update
   * @param udpate_args The update arguments
   * @return The pointer to the child with the update arguments
   */
  template<typename update, typename ...update_args>
  T* operator()(const uint64_t key, update&& u, update_args&&... args) {
    u(atomic::load(&stats_), std::forward<update_args>(args)...);
    child_type* c = get_or_create_child(key / CHILD_BLOCK);
    return (*c)(key % CHILD_BLOCK, u, std::forward<update_args>(args)...);
  }

  /**
   * Gets the child at the key
   * @param key The key for lookup
   * @return The child pointer
   */
  T* at(const uint64_t key) const {
    child_type* c = get_child(key / CHILD_BLOCK);
    return c->at(key % CHILD_BLOCK);
  }

  /**
   * Creates or gets the child at the key
   * @param k The key to create or fetch the child
   * @return The child
   */
  child_type* get_or_create_child(const uint64_t k) {
    return idx_.get_or_create(k);
  }

  /**
   * Gets the child at the key
   * @param k The key
   * @return The child at the key
   */
  child_type* get_child(const uint64_t k) const {
    return idx_.get(k);
  }

  /**
   * Gets the stats about the tiered index
   * @param key The key for lookup
   * @param depth The depth to look for child
   * @return The pointer to stats data
   */
  stats* get_stats(const uint64_t key, const uint64_t depth) {
    if (depth == 0)
      return get_stats();
    child_type* child = get_child(key / CHILD_BLOCK);
    if (child != nullptr)
      return child->get_stats(key % CHILD_BLOCK, depth - 1);
    return nullptr;
  }

  /**
   * Retrieves the stats
   * @return The pointer to stats data
   */
  stats* get_stats() {
    return atomic::load(&stats_);
  }

  template<typename update, typename ...update_args>
  void update_stats(const uint64_t key, update&& u, update_args&&... args) {
  }

 private:
  idx_type idx_;
  atomic::type<stats*> stats_;
};

template<typename T, size_t K, typename stats>
class tiered_index<T, K, 1, stats> {
 public:
  typedef T child_type;
  typedef indexlet<T, K> idx_type;

  tiered_index() {
    atomic::init(&stats_, static_cast<stats*>(nullptr));
  }

  T* operator[](const uint64_t key) {
    return get_or_create_child(key);
  }

  template<typename update, typename ...update_args>
  T* operator()(const uint64_t key, update&& u, update_args&&... args) {
    u(atomic::load(&stats_), std::forward<update_args>(args)...);
    return get_or_create_child(key);
  }

  T* at(const uint64_t key) const {
    return get_child(key);
  }

  child_type* get_or_create_child(const uint64_t k) {
    return idx_.get_or_create(k);
  }

  child_type* get_child(const uint64_t k) const {
    return idx_.get(k);
  }

  stats* get_stats(const uint64_t key, const uint64_t depth) {
    assert_throw(depth == 1, "depth = " << depth);
    return get_stats(key);
  }

  stats* get_stats(const uint64_t key) {
    return atomic::load(&stats_);
  }

 private:
  idx_type idx_;
  atomic::type<stats*> stats_;
};

template<typename T, size_t K, size_t D, typename S>
uint64_t tiered_index<T, K, D, S>::CHILD_BLOCK = math_utils::pow(K, D - 1);

}

}
#endif /* DIALOG_TIERED_INDEX_H_ */

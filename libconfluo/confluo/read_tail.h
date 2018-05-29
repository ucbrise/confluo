#ifndef CONFLUO_READ_TAIL_H_
#define CONFLUO_READ_TAIL_H_

#include <cstdint>
#include <thread>

#include "atomic.h"
#include "storage/storage.h"

namespace confluo {

/**
 * The read tail marker. Contains operations for modifying and setting
 * the marker for read requests.
 */
class read_tail {
 public:
  /**
   * Constructs a null read tail
   */
  read_tail();

  /**
   * Initializes a read tail based on the path of the data and the storage
   * mode
   *
   * @param data_path The data path
   * @param mode The storage mode
   */
  read_tail(const std::string &data_path, const storage::storage_mode &mode);

  /**
   * Initializes the read tail to the beginning
   *
   * @param data_path The data path of the read tail
   * @param mode The storage mode of the read tail
   */
  void init(const std::string &data_path, const storage::storage_mode &mode);

  /**
   * Loads the read tail marker
   *
   * @return The read tail marker
   */
  uint64_t get() const;

  /**
   * Advances the read tail marker by the specified number of bytes
   *
   * @param old_tail The old tail marker
   * @param bytes The number of bytes to advance the read tail marker
   */
  void advance(uint64_t old_tail, uint32_t bytes);

 private:
  atomic::type<uint64_t> *read_tail_{};
  storage::storage_mode mode_;
};

}

#endif /* CONFLUO_READ_TAIL_H_ */

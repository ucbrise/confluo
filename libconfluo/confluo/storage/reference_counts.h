#ifndef CONFLUO_STORAGE_REFERENCE_COUNTS_H_
#define CONFLUO_STORAGE_REFERENCE_COUNTS_H_

#include <atomic.h>

namespace confluo {
namespace storage {

/**
 * Packs two 16-bit ref counts in a single 32-bit integer.
 */
class reference_counts {
 public:
  reference_counts();

  reference_counts(reference_counts &other);

  reference_counts operator=(reference_counts &other);

  void increment_first();

  void increment_second();

  void increment_both();

  void decrement_first();

  void decrement_second();

  void decrement_both();

  bool decrement_first_and_compare();

  bool decrement_second_and_compare();

  uint32_t get_first();

  uint32_t get_second();

 private:
  atomic::type<uint32_t> ref_counts_;

  static const uint32_t FIRST_DELTA = 1U;
  static const uint32_t SECOND_DELTA = 1U << 16;
  static const uint32_t BOTH_DELTA = FIRST_DELTA + SECOND_DELTA;
  static const uint32_t FIRST_MASK = (1U << 16) - 1;
  static const uint32_t SECOND_SHIFT = 16;

};

}
}

#endif /* CONFLUO_STORAGE_REFERENCE_COUNTS_H_ */

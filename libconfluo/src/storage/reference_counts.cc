#include "storage/reference_counts.h"

namespace confluo {
namespace storage {

const uint32_t reference_counts::FIRST_DELTA;
const uint32_t reference_counts::SECOND_DELTA;
const uint32_t reference_counts::BOTH_DELTA;
const uint32_t reference_counts::FIRST_MASK;
const uint32_t reference_counts::SECOND_SHIFT;

reference_counts::reference_counts()
    : ref_counts_(BOTH_DELTA) {
}

reference_counts::reference_counts(reference_counts &other)
    : ref_counts_(atomic::load(&other.ref_counts_)) {
}

reference_counts reference_counts::operator=(reference_counts &other) {
  atomic::store(&ref_counts_, atomic::load(&other.ref_counts_));
  return *this;
}

void reference_counts::increment_first() {
  atomic::faa(&ref_counts_, FIRST_DELTA);
}

void reference_counts::increment_second() {
  atomic::faa(&ref_counts_, SECOND_DELTA);
}

void reference_counts::increment_both() {
  atomic::faa(&ref_counts_, BOTH_DELTA);
}

void reference_counts::decrement_first() {
  atomic::fas(&ref_counts_, FIRST_DELTA);
}

void reference_counts::decrement_second() {
  atomic::fas(&ref_counts_, SECOND_DELTA);
}

void reference_counts::decrement_both() {
  atomic::fas(&ref_counts_, BOTH_DELTA);
}

bool reference_counts::decrement_first_and_compare() {
  return (atomic::fas(&ref_counts_, 1U) & FIRST_MASK) == 1;
}

bool reference_counts::decrement_second_and_compare() {
  return (atomic::fas(&ref_counts_, SECOND_DELTA) >> SECOND_SHIFT) == 1;
}

uint32_t reference_counts::get_first() {
  return atomic::load(&ref_counts_) & FIRST_MASK;
}

uint32_t reference_counts::get_second() {
  return atomic::load(&ref_counts_) >> SECOND_SHIFT;
}

}
}
#ifndef SPLITORDERED_DEFS_H_
#define SPLITORDERED_DEFS_H_

#include <cstdint>
#include <cstdlib>

namespace splitordered {

typedef uint64_t key_t;
typedef uint64_t so_key_t;

template<class data_type>
struct hash_entry {
  so_key_t key;
  std::atomic<data_type> value;
  hash_entry<data_type>* next;
};

/* These are GCC builtins; other compilers may require inline assembly */
#define CAS(ADDR, OLDV, NEWV) __sync_val_compare_and_swap((ADDR), (OLDV), (NEWV))

#define UNINITIALIZED        NULL

#define MSB (((uint64_t)1) << 63)
#define REVERSE_BYTE(x) ((so_key_t)((((((uint32_t)(x)) * 0x0802LU & 0x22110LU) | (((uint32_t)(x)) * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16) & 0xff))
#define REVERSE(x) ((REVERSE_BYTE((((so_key_t)(x))) & 0xff) << 56) | \
    (REVERSE_BYTE((((so_key_t)(x)) >> 8) & 0xff) << 48) | \
    (REVERSE_BYTE((((so_key_t)(x)) >> 16) & 0xff) << 40) | \
    (REVERSE_BYTE((((so_key_t)(x)) >> 24) & 0xff) << 32) | \
    (REVERSE_BYTE((((so_key_t)(x)) >> 32) & 0xff) << 24) | \
    (REVERSE_BYTE((((so_key_t)(x)) >> 40) & 0xff) << 16) | \
    (REVERSE_BYTE((((so_key_t)(x)) >> 48) & 0xff) << 8) | \
    (REVERSE_BYTE((((so_key_t)(x)) >> 56) & 0xff) << 0))

#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))
}

#endif /* SPLITORDERED_DEFS_H_ */

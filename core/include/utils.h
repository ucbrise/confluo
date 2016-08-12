#ifndef SLOG_UTILS_H_
#define SLOG_UTILS_H_

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/mman.h>

namespace slog {

class file_utils {
 public:
  static char* memory_map_mutable(const char* path, size_t size) {
    int fd = open(path, (O_CREAT | O_TRUNC | O_RDWR),
                  (S_IRWXU | S_IRWXG | S_IRWXO));

    if (fd < 0) {
      fprintf(stderr, "Could not obtain file descriptor.\n");
      throw -1;
    }

    off_t lastoffset = lseek(fd, size, SEEK_SET);
    const char eof[1] = { 0 };
    size_t bytes_written = write(fd, eof, 1);

    if (bytes_written != 1) {
      fprintf(stderr, "Could not write to file.\n");
      throw -1;
    }

    char* data = (char *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                               fd, 0);
    if (data == MAP_FAILED) {
      fprintf(stderr, "Could not mmap file.\n");
      throw -1;
    }

    return data;
  }
};

class hash_utils {
 public:
  static const uint32_t K1 = 256;
  static const uint32_t K2 = 65536;
  static const uint32_t K3 = 16777216;

  static uint32_t simple_hash(const char* buf) {
    uint8_t* cast_buf = (uint8_t*) buf;
#ifdef HASH4
    return cast_buf[0] * K3 + cast_buf[1] * K2 + cast_buf[2] * K1 + cast_buf[3];
#else
#ifdef HASH3
    return cast_buf[0] * K2 + cast_buf[1] * K1 + cast_buf[2];
#else
    return cast_buf[0] * K1 + cast_buf[1];
#endif
#endif
  }
};

class bit_utils {
 public:
  static inline uint32_t highest_bit(uint32_t x) {
    uint32_t y = 0;
#ifdef BSR
    asm ( "\tbsr %1, %0\n"
        : "=r"(y)
        : "r" (x)
    );
#else
    while (x >>= 1)
      ++y;
#endif
    return y;
  }
};

}

#endif /* SLOG_UTILS_H_ */

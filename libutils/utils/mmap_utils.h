#ifndef UTILS_MMAP_UTILS_H_
#define UTILS_MMAP_UTILS_H_

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#include "file_utils.h"
#include "assertions.h"

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

namespace utils {

class mmap_utils {
 public:
  static const int PERMISSIONS = (S_IRWXU | S_IRWXG | S_IRWXO);
  static const int PROT_RW = PROT_READ | PROT_WRITE;
  static const int FLAGS = MAP_SHARED;

  static void* map(int fd, void *addr_hint, size_t offset, size_t size, int prot = PROT_RW, int flags = FLAGS);

  static void unmap(void *addr, size_t size);

  static void flush(void *addr, size_t size);
};

}

#endif /* UTILS_MMAP_UTILS_H_ */

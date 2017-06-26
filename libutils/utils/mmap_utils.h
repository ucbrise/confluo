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

  static void* map(int fd, void *addr_hint, size_t offset, size_t size,
                   int prot = PROT_RW, int flags = FLAGS) {
    void* data = mmap(addr_hint, size, prot, flags, fd, offset);
    assert_throw(
        data != MAP_FAILED,
        "mmap(" << addr_hint << ", " << size << ", " << prot << ", " << flags << ", " << fd << ", " << offset << "): " << strerror(errno));
    return data;
  }

  static void unmap(void *addr, size_t size) {
    int ret = munmap(addr, size);
    assert_throw(ret != -1,
                 "munmap(" << addr << ", " << size << "): " << strerror(errno));
  }

  static void flush(void *addr, size_t size) {
    if (size == 0)
      return;
    static size_t page_size = sysconf(_SC_PAGESIZE);
    size_t off = (size_t) addr % page_size;
    int ret = msync(((char*) addr) - off, size + off, MS_SYNC);
    assert_throw(ret != -1,
                 "msync(" << addr << ", " << size << "): " << strerror(errno));
  }
};

}

#endif /* UTILS_MMAP_UTILS_H_ */

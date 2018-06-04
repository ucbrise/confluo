#include "mmap_utils.h"

namespace utils {

const int mmap_utils::PERMISSIONS;
const int mmap_utils::PROT_RW;
const int mmap_utils::FLAGS;

void *mmap_utils::map(int fd, void *addr_hint, size_t offset, size_t size, int prot, int flags) {
  void *data = mmap(addr_hint, size, prot, flags, fd, static_cast<off_t>(offset));
  assert_throw(
      data != MAP_FAILED,
      "mmap(" << addr_hint << ", " << size << ", " << prot << ", " << flags << ", " << fd << ", " << offset << "): "
              << strerror(errno));
  return data;
}

void mmap_utils::unmap(void *addr, size_t size) {
  int ret = munmap(addr, size);
  assert_throw(ret != -1, "munmap(" << addr << ", " << size << "): " << strerror(errno));
}

void mmap_utils::flush(void *addr, size_t size) {
  if (size == 0)
    return;
  static size_t page_size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
  size_t off = (size_t) addr % page_size;
  int ret = msync(((char *) addr) - off, size + off, MS_SYNC);
  assert_throw(ret != -1, "msync(" << addr << ", " << size << "): " << strerror(errno));
}

}
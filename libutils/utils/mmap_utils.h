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

#include "assertions.h"

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

namespace utils {

class mmap_utils {
 public:
  static size_t file_size(const std::string& path) {
    struct stat st;
    stat(path.c_str(), &st);
    return st.st_size;
  }

  static void* mmap_r(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY, (S_IRWXU | S_IRWXG | S_IRWXO));
    assert_throw(fd >= 0,
                 "Could not open file " << path << ": " << strerror(errno));

    void* data = mmap(NULL, file_size(path), PROT_READ,
    MAP_SHARED | MAP_POPULATE,
                      fd, 0);
    assert_throw(data != MAP_FAILED,
                 "Could not mmap file " << path << ": " << strerror(errno));

    return data;
  }

  static void* mmap_rw(const std::string path, size_t size) {
    int fd = open(path.c_str(), (O_CREAT | O_TRUNC | O_RDWR),
                  (S_IRWXU | S_IRWXG | S_IRWXO));

    assert_throw(fd >= 0,
                 "Could not open file " << path << ":" << strerror(errno));

    const char eof[1] = { 0 };
    off_t end = lseek(fd, size, SEEK_SET);
    assert_throw(end != -1,
                 "Could not lseek file " << path << ": " << strerror(errno));

    size_t nbytes = write(fd, eof, sizeof(char));
    assert_throw(nbytes == 1,
                 "Could not write to file " << path << ": " << strerror(errno));

    void* data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert_throw(data != MAP_FAILED,
                 "Could not mmap file " << path << ": " << strerror(errno));

    return data;
  }

  static void* mmap_rw_init(const std::string path, size_t size, int byte) {
    int fd = open(path.c_str(), (O_CREAT | O_TRUNC | O_RDWR),
                  (S_IRWXU | S_IRWXG | S_IRWXO));

    assert_throw(fd >= 0,
                 "Could not open file " << path << ": " << strerror(errno));

    const char eof[1] = { 0 };
    off_t end = lseek(fd, size, SEEK_SET);
    assert_throw(end != -1,
                 "Could not lseek file " << path << ": " << strerror(errno));

    size_t nbytes = write(fd, eof, sizeof(char));
    assert_throw(nbytes == 1,
                 "Could not write to file " << path << ": " << strerror(errno));

    void* data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert_throw(data != MAP_FAILED,
                 "Could not mmap file " << path << ": " << strerror(errno));

    memset(data, byte, size);
    return data;
  }

  static void mmap_flush(void *addr, size_t size) {
    if (size == 0)
      return;
    static size_t page_size = sysconf(_SC_PAGESIZE);
    size_t off = (size_t) addr % page_size;
    int ret = msync(((char*) addr) - off, size + off, MS_SYNC);
    assert_throw(ret != -1, "msync failed: " << strerror(errno));
  }
};

}

#endif /* UTILS_MMAP_UTILS_H_ */

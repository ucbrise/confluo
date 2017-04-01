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

#include "assertions.h"

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

namespace utils {

class mmap_utils {
 public:
  static void* mmap_r(const std::string path) {
    int fd = open(path.c_str(), O_RDONLY, (S_IRWXU | S_IRWXG | S_IRWXO));
    assert_throw(fd >= 0, "Could not open file " << path << " for mmap");

    struct stat st;
    stat(path.c_str(), &st);
    size_t size = st.st_size;

    void* data = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, fd, 0);
    assert_throw(data != MAP_FAILED, "Could not mmap file " << path);

    return data;
  }

  static void* mmap_rw(const std::string path, size_t size) {
    int fd = open(path.c_str(), (O_CREAT | O_TRUNC | O_RDWR),
                  (S_IRWXU | S_IRWXG | S_IRWXO));

    assert_throw(fd >= 0, "Could not open file " << path << " for mmap");

    const char eof[1] = { 0 };
    off_t end = lseek(fd, size, SEEK_SET);
    assert_throw(end != -1, "Could not seek to end of file " << path);

    size_t nbytes = write(fd, eof, sizeof(char));
    assert_throw(nbytes == 1, "Could not write to file " << path);

    void* data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert_throw(data != MAP_FAILED, "Could not mmap file " << path);

    return data;
  }

  static void* mmap_rw_init(const std::string path, size_t size, int byte) {
    int fd = open(path.c_str(), (O_CREAT | O_TRUNC | O_RDWR),
                  (S_IRWXU | S_IRWXG | S_IRWXO));

    assert_throw(fd >= 0, "Could not open file " << path << " for mmap");

    const char eof[1] = { 0 };
    off_t end = lseek(fd, size, SEEK_SET);
    assert_throw(end != -1, "Could not seek to end of file " << path);

    size_t nbytes = write(fd, eof, sizeof(char));
    assert_throw(nbytes == 1, "Could not write to file " << path);

    void* data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert_throw(data != MAP_FAILED, "Could not mmap file " << path);

    memset(data, byte, size);
    return data;
  }
};

}

#endif /* UTILS_MMAP_UTILS_H_ */

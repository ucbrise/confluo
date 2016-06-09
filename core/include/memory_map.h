#ifndef MEMORY_MAP_H_
#define MEMORY_MAP_H_

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/mman.h>

class MemoryMap {
 public:
  static char* map(const char* path, size_t size) {
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

#endif /* MEMORY_MAP_H_ */

/*
 * mmap_utils.h
 *
 *  Created on: Mar 21, 2017
 *      Author: anuragk
 */

#ifndef UTILS_MMAP_UTILS_H_
#define UTILS_MMAP_UTILS_H_

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/mman.h>

#include "assertions.h"

namespace utils {

class mmap_utils {
 public:
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
};

}

#endif /* UTILS_MMAP_UTILS_H_ */

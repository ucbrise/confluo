#ifndef UTILS_FILE_UTILS_H_
#define UTILS_FILE_UTILS_H_

#include <unistd.h>
#include <cstdlib>
#include <string>
#include <errno.h>

#include "assertions.h"

namespace utils {

class file_utils {
 public:
  static const int PERMISSIONS = (S_IRWXU | S_IRWXG | S_IRWXO);

  static size_t file_size(const std::string& path) {
    struct stat st;
    stat(path.c_str(), &st);
    return st.st_size;
  }

  static int create_file(const std::string& path, int flags) {
    int ret = open(path.c_str(), flags, PERMISSIONS);
    assert_throw(
        ret != -1,
        "open(" << path << ", " << flags << "," << PERMISSIONS << "): " << strerror(errno));
    return ret;
  }

  static void truncate_file(std::string& path, size_t size) {
    int ret = truncate(path.c_str(), size);
    assert_throw(
        ret != -1,
        "truncate(" << path << ", " << size << "): " << strerror(errno));
  }

  static void truncate_file(int fd, size_t size) {
    int ret = ftruncate(fd, size);
    assert_throw(
        ret != -1,
        "ftruncate(" << fd << ", " << size << "): " << strerror(errno));
  }

  static void close_file(int fd) {
    int ret = close(fd);
    assert_throw(ret != -1, "close(" << fd << "): " << strerror(errno));
  }
};

}

#endif /* UTILS_FILE_UTILS_H_ */

#ifndef UTILS_FILE_UTILS_H_
#define UTILS_FILE_UTILS_H_

#include <unistd.h>
#include <cstdlib>
#include <string>
#include <errno.h>
#include <climits>
#include <fcntl.h>
#include <sys/stat.h>

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

  static void create_dir(const std::string& path) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path.c_str());
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
      tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
      if (*p == '/') {
        *p = 0;
        mkdir(tmp, S_IRWXU);
        *p = '/';
      }
    mkdir(tmp, S_IRWXU);
  }

  static bool exists_file(const std::string& path) {
    struct stat buf;
    return stat(path.c_str(), &buf) == 0;
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

  static std::string full_path(const std::string& path) {
    char full_path[4096];
    realpath(path.c_str(), full_path);
    return std::string(full_path);
  }
};

}

#endif /* UTILS_FILE_UTILS_H_ */

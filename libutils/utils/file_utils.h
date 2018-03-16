#ifndef UTILS_FILE_UTILS_H_
#define UTILS_FILE_UTILS_H_

#include <stdio.h>
#include <dirent.h>

#include <unistd.h>
#include <cstdlib>
#include <string>
#include <errno.h>
#include <ftw.h>
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

  static void clear_dir(const std::string& path) {
    delete_dir(path);
    create_dir(path);
  }

  static void delete_dir(const std::string& path) {
    nftw(path.c_str(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
  }

  static bool exists_file(const std::string& path) {
    struct stat buf;
    return stat(path.c_str(), &buf) == 0;
  }

  static int delete_file(const std::string& path) {
    int ret = remove(path.c_str());
    assert_throw(ret != -1, "remove(" << path << "):" << strerror(errno));
    return ret;
  }

  static int open_file(const std::string& path, int flags) {
    int ret = open(path.c_str(), flags, PERMISSIONS);
    assert_throw(
        ret != -1,
        "open(" << path << ", " << flags << "," << PERMISSIONS << "): " << strerror(errno));
    return ret;
  }

  static void truncate_file(const std::string& path, size_t size) {
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

 private:
  static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
      int ret = remove(fpath);
      assert_throw(ret != -1, "remove(" << fpath << "):" << strerror(errno));
      return ret;
  }

};

}

#endif /* UTILS_FILE_UTILS_H_ */

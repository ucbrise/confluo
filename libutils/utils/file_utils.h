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

  static size_t file_size(const std::string &path);

  static void create_dir(const std::string &path);

  static void clear_dir(const std::string &path);

  static void delete_dir(const std::string &path);

  static bool exists_file(const std::string &path);

  static int delete_file(const std::string &path);

  static int open_file(const std::string &path, int flags);

  static void truncate_file(const std::string &path, size_t size);

  static void truncate_file(int fd, size_t size);

  static void close_file(int fd);

  static std::string full_path(const std::string &path);

 private:
  static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);

};

}

#endif /* UTILS_FILE_UTILS_H_ */

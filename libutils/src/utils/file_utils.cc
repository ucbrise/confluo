#include "file_utils.h"

namespace utils {

const int file_utils::PERMISSIONS;

size_t file_utils::file_size(const std::string &path) {
  struct stat st;
  stat(path.c_str(), &st);
  return static_cast<size_t>(st.st_size);
}
void file_utils::create_dir(const std::string &path) {
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
void file_utils::clear_dir(const std::string &path) {
  delete_dir(path);
  create_dir(path);
}
void file_utils::delete_dir(const std::string &path) {
  nftw(path.c_str(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}
bool file_utils::exists_file(const std::string &path) {
  struct stat buf;
  return stat(path.c_str(), &buf) == 0;
}
int file_utils::delete_file(const std::string &path) {
  int ret = remove(path.c_str());
  assert_throw(ret != -1, "remove(" << path << "):" << strerror(errno));
  return ret;
}
int file_utils::open_file(const std::string &path, int flags) {
  int ret = open(path.c_str(), flags, PERMISSIONS);
  assert_throw(
      ret != -1,
      "open(" << path << ", " << flags << "," << PERMISSIONS << "): " << strerror(errno));
  return ret;
}
void file_utils::truncate_file(const std::string &path, size_t size) {
  int ret = truncate(path.c_str(), static_cast<off_t>(size));
  assert_throw(
      ret != -1,
      "truncate(" << path << ", " << size << "): " << strerror(errno));
}
void file_utils::truncate_file(int fd, size_t size) {
  int ret = ftruncate(fd, size);
  assert_throw(
      ret != -1,
      "ftruncate(" << fd << ", " << size << "): " << strerror(errno));
}
void file_utils::close_file(int fd) {
  int ret = close(fd);
  assert_throw(ret != -1, "close(" << fd << "): " << strerror(errno));
}
std::string file_utils::full_path(const std::string &path) {
  char full_path[4096];
  realpath(path.c_str(), full_path);
  return std::string(full_path);
}
int file_utils::unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
  int ret = remove(fpath);
  assert_throw(ret != -1, "remove(" << fpath << "):" << strerror(errno));
  return ret;
}
}
#ifndef CONFLUO_ARCHIVAL_IO_INCR_FILE_OFFSET_H_
#define CONFLUO_ARCHIVAL_IO_INCR_FILE_OFFSET_H_

#include <string>

namespace confluo {
namespace archival {

/**
 * Offset into an incremental file, uniquely defined by
 * the path to a file and the offset into the file
 */
class incremental_file_offset {
 public:
  incremental_file_offset();

  incremental_file_offset(std::string path, size_t offset);

  std::string path() const;

  size_t offset() const;

 private:
  std::string path_;
  size_t off_;
};

}
}

#endif /* CONFLUO_ARCHIVAL_IO_INCR_FILE_OFFSET_H_ */

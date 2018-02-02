#ifndef CONFLUO_ARCHIVAL_IO_INCR_FILE_OFFSET_H_
#define CONFLUO_ARCHIVAL_IO_INCR_FILE_OFFSET_H_

namespace confluo {
namespace archival {

class incremental_file_offset {
 public:
  incremental_file_offset()
      : path_(""),
        off_(0) {
  }

  incremental_file_offset(std::string path, size_t offset)
      : path_(path),
        off_(offset) {
  }

  std::string path() const {
    return path_;
  }

  size_t offset() const {
    return off_;
  }

  size_t advance(size_t increment) {
    off_ += increment;
    return off_;
  }

 private:
  std::string path_;
  size_t off_;
};

}
}

#endif /* CONFLUO_ARCHIVAL_IO_INCR_FILE_OFFSET_H_ */

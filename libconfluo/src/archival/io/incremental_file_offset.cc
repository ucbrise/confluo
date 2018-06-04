#include "archival/io/incremental_file_offset.h"

namespace confluo {
namespace archival {

incremental_file_offset::incremental_file_offset()
    : path_(""),
      off_(0) {
}

incremental_file_offset::incremental_file_offset(std::string path, size_t offset)
    : path_(path),
      off_(offset) {
}

std::string incremental_file_offset::path() const {
  return path_;
}

size_t incremental_file_offset::offset() const {
  return off_;
}

}
}
#ifndef CONFLUO_ARCHIVAL_IO_INCR_FILE_UTILS_H_
#define CONFLUO_ARCHIVAL_IO_INCR_FILE_UTILS_H_

#include "incr_file_offset.h"

namespace confluo {
namespace archival {

class incr_file_utils {

 public:
  static void truncate_rest(incremental_file_offset off) {
    // TODO truncate succeeding files as well
    std::string path = off.path();
    file_utils::truncate_file(path, off.offset());
  }

};

}
}

#endif /* CONFLUO_ARCHIVAL_IO_INCR_FILE_UTILS_H_ */

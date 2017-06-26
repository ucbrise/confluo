#ifndef DIALOG_LOG_MANAGER_H_
#define DIALOG_LOG_MANAGER_H_

#include "atomic.h"
#include "mmap_utils.h"

namespace dialog {

#define LOG_ID_C(x) UINT32_C(x)
typedef uint32_t log_id_t;

class log_manager {
 public:
  static atomic::type<log_id_t> LOG_ID;

  /**
   * Request a log of 'bytes' capacity.
   *
   * @param bytes Capacity in bytes of the requested log.
   * @return Descriptor to the allocated log.
   */
  static log_id_t allocate_log(size_t bytes) {
    log_id_t id = atomic::faa(&LOG_ID, LOG_ID_C(1));
    std::string path = get_filename(id);
    int fd = open(path.c_str(), (O_CREAT | O_TRUNC | O_RDWR),
                  (S_IRWXU | S_IRWXG | S_IRWXO));
    assert_throw(fd >= 0,
                 "Could not open file " << path << ": " << strerror(errno));

    int ret = ftruncate(fd, bytes);
    assert_throw(ret == 0,
                 "Could not truncate file " << path << ": " << strerror(errno));

    ret = close(fd);
    assert_throw(ret == 0,
                 "Could not close file " << path << ": " << strerror(errno));

    return id;
  }

  /**
   * Get filename for a log, given its id.
   * @param id
   * @return
   */
  static const std::string get_filename(log_id_t id) {
    return "data-" + std::to_string(id) + ".log";
  }
};

atomic::type<log_id_t> log_manager::LOG_ID(0);

}

#endif /* DIALOG_LOG_MANAGER_H_ */

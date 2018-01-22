#ifndef CONFLUO_ARCHIVAL_ARCHIVAL_CONSTS_H_
#define CONFLUO_ARCHIVAL_ARCHIVAL_CONSTS_H_

namespace confluo {
namespace archival {

class archival_utils {
 public:
  static std::string filter_archival_path(const std::string& filter_log_path, size_t filter_log_idx) {
    return filter_log_path + "/filter_" + std::to_string(filter_log_idx) + "/";
  }

  static std::string index_archival_path(const std::string& index_log_path, size_t index_log_idx) {
    return index_log_path + "/index_" + std::to_string(index_log_idx) + "/";
  }

};

}
}

#endif /* CONFLUO_ARCHIVAL_ARCHIVAL_CONSTS_H_ */

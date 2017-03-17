#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <cstdio>
#include <ctime>

namespace utils {

class time_utils {
 public:
  static std::string current_date_time() {
    std::time_t rawtime;
    std::tm* timeinfo;
    char buffer[80];

    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d.%X", timeinfo);
    return std::string(buffer);
  }
};

}

#endif /* UTILS_UTILS_H_ */

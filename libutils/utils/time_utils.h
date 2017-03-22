#ifndef UTILS_TIME_UTILS_H_
#define UTILS_TIME_UTILS_H_

#include <cstdio>
#include <ctime>

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

#endif /* LIBUTILS_UTILS_TIME_UTILS_H_ */

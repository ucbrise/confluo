#ifndef UTILS_ERROR_HANDLING_H_
#define UTILS_ERROR_HANDLING_H_

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <sstream>

#include <sys/wait.h>
#include <unistd.h>

namespace utils {

class error_handling {
 public:
  static const uint32_t MAX_FRAMES = 64;

  static inline void install_signal_handler(const char *exec) {}

  template<typename ...args>
  static inline void install_signal_handler(const char *exec, int sig,
                                            args ... more) {
    signal(sig, error_handling::sighandler_stacktrace);
    error_handling::install_signal_handler(exec, std::forward<int>(more)...);
  }

  static std::string stacktrace();

 private:
  static void sighandler_stacktrace(int sig);
};

}

#endif /* UTILS_ERROR_HANDLING_H_ */

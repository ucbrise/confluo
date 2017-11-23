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
  static char exec_path[1024];

  static inline void install_signal_handler(const char* exec) {
    realpath(exec, exec_path);
  }

  template<typename ...args>
  static inline void install_signal_handler(const char* exec, int sig,
                                            args ... more) {
    signal(sig, error_handling::sighandler_stacktrace);
    error_handling::install_signal_handler(exec, std::forward<int>(more)...);
  }

  static std::string stacktrace() {
    std::ostringstream out;
    out << "stack trace (executable: " << exec_path << "): \n";

    // storage array for stack trace address data
    char buf[1024];
    void* addr_list[MAX_FRAMES + 1];

    // retrieve current stack addresses
    int addr_len = backtrace(addr_list, sizeof(addr_list) / sizeof(void*));

    if (addr_len == 0) {
      out << "  <empty, possibly corrupt>\n";
      return out.str();
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbol_list = backtrace_symbols(addr_list, addr_len);
    for (int i = 1; i < addr_len; i++) {
      Dl_info info;
      if (dladdr(addr_list[i], &info) && info.dli_sname) {
        char *demangled = NULL;
        int status = -1;
        if (info.dli_sname[0] == '_') {
          demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
        }
        snprintf(
            buf,
            sizeof(buf),
            "%-3d %*p %s+%zd",
            i,
            int(2 + sizeof(void*) * 2),
            addr_list[i],
            status == 0 ? demangled :
            info.dli_sname == 0 ? symbol_list[i] : info.dli_sname,
            (char *) addr_list[i] - (char *) info.dli_saddr);
        if (demangled)
          free(demangled);
      } else {
        snprintf(buf, sizeof(buf), "%-3d %*p %s", i, int(2 + sizeof(void*) * 2),
                 addr_list[i], symbol_list[i]);
      }
      out << buf;

      char atos_cmd[256];
#ifdef __linux__
      sprintf(atos_cmd, "addr2line %p -e %s", addr_list[i], exec_path);
#else
#ifdef __APPLE__
      sprintf(atos_cmd, "gaddr2line %p -e %s", addr_list[i], exec_path);
#endif
#endif
      FILE* cmd = popen(atos_cmd, "r");
      if (cmd) {
        char buf[256];
        fscanf(cmd, "%256s", buf);
        pclose(cmd);
        out << " (" << buf << ")\n";
      } else {
        out << "\n";
      }
    }

    free(symbol_list);
    return out.str();
  }

 private:
  static inline void sighandler_stacktrace(int sig) {
    fprintf(stderr, "ERROR: signal %d\n", sig);
    fprintf(stderr, "%s\n", stacktrace().c_str());
    exit(-1);
  }
};

char error_handling::exec_path[1024];

}

#endif /* UTILS_ERROR_HANDLING_H_ */

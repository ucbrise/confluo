#ifndef UTILS_ERROR_HANDLING_H_
#define UTILS_ERROR_HANDLING_H_

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

namespace utils {

class error_handling {
 public:
  static const uint32_t MAX_FRAMES = 64;

  static inline void install_signal_handler() {
  }

  template<typename ...args>
  static inline void install_signal_handler(int sig, args ... more) {
    signal(sig, error_handling::sighandler_stacktrace);
    error_handling::install_signal_handler(std::forward<int>(more)...);
  }

  static std::string stacktrace() {
    std::ostringstream out;
    out << "stack trace: \n";

    // storage array for stack trace address data
    void* addrlist[MAX_FRAMES + 1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0) {
      out << "  <empty, possibly corrupt>\n";
      return out.str();
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char* funcname = (char*) malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (int i = 1; i < addrlen; i++) {
      char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

      // find parentheses and +address offset surrounding the mangled name:
      // ./module(function+0x15c) [0x8048a6d]
      for (char *p = symbollist[i]; *p; ++p) {
        if (*p == '(')
          begin_name = p;
        else if (*p == '+')
          begin_offset = p;
        else if (*p == ')' && begin_offset) {
          end_offset = p;
          break;
        }
      }

      if (begin_name && begin_offset && end_offset
          && begin_name < begin_offset) {
        *begin_name++ = '\0';
        *begin_offset++ = '\0';
        *end_offset = '\0';

        // mangled name is now in [begin_name, begin_offset) and caller
        // offset in [begin_offset, end_offset). now apply
        // __cxa_demangle():

        int status;
        char* ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize,
                                        &status);
        if (status == 0) {
          funcname = ret;  // use possibly realloc()-ed string
          out << "  " << symbollist[i] << ": " << funcname << "+"
              << begin_offset;
        } else {
          // demangling failed. Output function name as a C function with
          // no arguments.
          out << "  " << symbollist[i] << ": " << begin_name << "()+"
              << begin_offset;
        }

        char syscom[256];
        sprintf(syscom, "addr2line %p -e %s", addrlist[i], symbollist[i]);
        FILE* cmd = popen(syscom, "r");
        if (cmd) {
          char buf[256];
          fscanf(cmd, "%256s", buf);
          pclose(cmd);
          out << "(" << buf << ")\n";
        } else {
          out << "\n";
        }
      } else {
        // couldn't parse the line? print the whole line.
        out << "  " << symbollist[i] << "\n";
      }
    }

    free(funcname);
    free(symbollist);
    return out.str();
  }

 private:
  static inline void sighandler_stacktrace(int sig) {
    fprintf(stderr, "ERROR: signal %d\n", sig);
    fprintf(stderr, "%s\n", stacktrace().c_str());
    exit(-1);
  }

  static inline void gdb_stack_trace(int sig) {
    char pid_buf[30];
    sprintf(pid_buf, "%d", getpid());
    char name_buf[512];
    name_buf[readlink("/proc/self/exe", name_buf, 511)] = 0;
    int child_pid = fork();
    if (!child_pid) {
      dup2(2, 1);  // redirect output to stderr
      fprintf(stderr, "ERROR: signal %d\n", sig);
      fprintf(stderr, "stack trace for %s pid=%s\n", name_buf, pid_buf);
      execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt",
             name_buf, pid_buf, NULL);
      abort(); /* If gdb failed to start */
    } else {
      waitpid(child_pid, NULL, 0);
    }
    exit(-1);
  }
};

}

#endif /* UTILS_ERROR_HANDLING_H_ */

#ifndef PROFILER_H_
#define PROFILER_H_

#include <sstream>
#include <iostream>
#include <functional>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

struct Profiler {
  static void profile(const std::string& name, std::function<void()> body) {
    std::string filename =
        name.find(".profile") == std::string::npos ? (name + ".profile") : name;

    // Launch profiler
    pid_t pid;
    std::stringstream s;
    s << getpid();
    pid = fork();
    if (pid == 0) {
      auto fd = open("/dev/null", O_RDWR);
      dup2(fd, 1);
      dup2(fd, 2);
      exit(
          execl("/usr/bin/perf", "perf", "record", "-o", filename.c_str(), "-p",
                s.str().c_str(), nullptr));
    }

    // Run body
    body();

    // Kill profiler
    kill(pid, SIGINT);
    waitpid(pid, nullptr, 0);
  }

  static void profile(std::function<void()> body) {
    profile("perf.data", body);
  }
};

#endif /* PROFILER_H_ */

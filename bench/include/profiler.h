#ifndef PROFILER_H_
#define PROFILER_H_

#include <iostream>
#include <functional>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

struct Profiler {
  static void profile(std::function<void()> body) {
    // Launch profiler
    pid_t pid;
    pid = fork();
    if (pid == 0) {
      auto fd = open("/dev/null", O_RDWR);
      dup2(fd, 1);
      dup2(fd, 2);
      exit(
          execl("/usr/bin/callgrind_control", "callgrind_control", "-i", "on",
                nullptr));
    }
    waitpid(pid, nullptr, 0);

    // Run body
    body();

    pid = fork();
    if (pid == 0) {
      auto fd = open("/dev/null", O_RDWR);
      dup2(fd, 1);
      dup2(fd, 2);
      exit(
          execl("/usr/bin/callgrind_control", "callgrind_control", "-i", "off",
                nullptr));
    }
    waitpid(pid, nullptr, 0);

  }
};

#endif /* PROFILER_H_ */

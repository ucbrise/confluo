#ifndef PROFILER_H_
#define PROFILER_H_

#include <stdlib.h>

struct Profiler {
  static void StartProfiling() {
    system("callgrind_control -i on");
  }

  static void StopProfiling() {
    system("callgrind_control -i off");
  }
};

#endif /* PROFILER_H_ */

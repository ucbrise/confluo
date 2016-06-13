#ifndef PROFILER_H_
#define PROFILER_H_

#include <stdlib.h>
#include "gperftools/profiler.h"

struct Profiler {
  static void StartProfiling(const char* name = "profile.dat") {
    ProfilerStart(name);
  }

  static void StopProfiling() {
    ProfilerFlush();
    ProfilerStop();
  }
};

#endif /* PROFILER_H_ */

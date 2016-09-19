#ifndef BENCH_CPU_UTILIZATION_H_
#define BENCH_CPU_UTILIZATION_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/times.h>

class cpu_utilization {
 public:
  cpu_utilization() {
    struct tms time_sample;
    last_cpu = times(&time_sample);
    last_sys_cpu = time_sample.tms_stime;
    last_user_cpu = time_sample.tms_utime;
  }

  double current() {
    struct tms time_sample;
    clock_t now;
    double percent;

    now = times(&time_sample);
    if (now <= last_cpu || time_sample.tms_stime < last_sys_cpu
        || time_sample.tms_utime < last_user_cpu) {
      //Overflow detection. Just skip this value.
      percent = -1.0;
    } else {
      percent = (time_sample.tms_stime - last_sys_cpu)
          + (time_sample.tms_utime - last_user_cpu);
      percent /= (now - last_cpu);
      percent *= 100;
    }
    last_cpu = now;
    last_sys_cpu = time_sample.tms_stime;
    last_user_cpu = time_sample.tms_utime;

    return percent;
  }

 private:
  clock_t last_cpu, last_sys_cpu, last_user_cpu;
};

#endif /* BENCH_CPU_UTILIZATION_H_ */

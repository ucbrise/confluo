#ifndef UTILS_BENCHMARK_H_
#define UTILS_BENCHMARK_H_

#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "logger.h"

#define LOOP_OPS(op, ds, num_ops, limit) \
  num_ops = 0;\
  while (num_ops < limit) {\
    op(ds);\
    num_ops++;\
  }

#ifdef _GNU_SOURCE
#define SET_CORE_AFFINITY(t, core_id)\
  LOG_INFO << "Pinning thread to core" << core_id;\
  cpu_set_t cpuset;\
  CPU_ZERO(&cpuset);\
  CPU_SET(core_id, &cpuset);\
  int rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);\
  if (rc != 0)\
    LOG_WARN << "Error calling pthread_setaffinity_np: " << rc;
#else
#define SET_CORE_AFFINITY(thread, core_id)\
  LOG_WARN << "Not pinning thread to core";
#endif

#define BENCH_OP(op, nthreads)\
  std::string output_file = this->output_dir_ + "/throughput-" + #op + "-"\
     + std::to_string(nthreads) + ".txt";\
  this->bench_thput(op, nthreads, output_file);

#define DEFINE_BENCH(op)\
  void bench_##op(size_t nthreads) {\
    BENCH_OP(op, nthreads)\
  }

namespace utils {
namespace bench {

struct constants {
  static const uint64_t WARMUP_OPS = 5000000;
  static const uint64_t MEASURE_OPS = 10000000;
  static const uint64_t COOLDOWN_OPS = 5000000;
};

typedef std::chrono::high_resolution_clock timer;
typedef std::chrono::microseconds us;

template<typename data_structure, typename op_t>
static void bench_thput_thread(op_t&& op, data_structure& ds, size_t nthreads,
                               size_t i, std::vector<double>& thput) {
  size_t num_ops;

  LOG_INFO<< "Running warmup for " << constants::WARMUP_OPS << " ops";
  LOOP_OPS(op, ds, num_ops, constants::WARMUP_OPS);

  LOG_INFO<< "Warmup complete; running measure for " << constants::MEASURE_OPS << " ops";
  auto start = timer::now();
  LOOP_OPS(op, ds, num_ops, constants::MEASURE_OPS);
  auto end = timer::now();

  auto usecs = std::chrono::duration_cast<us>(end - start).count();
  assert(usecs > 0);
  thput[i] = num_ops * 1e6 / usecs;

  LOG_INFO<< "Measure complete; running cooldown for " << constants::COOLDOWN_OPS << " ops";
  LOOP_OPS(op, ds, num_ops, constants::COOLDOWN_OPS);

  LOG_INFO<< "Thread completed benchmark at " << thput[i] << "ops/s";
}

template<typename data_structure>
class benchmark {
 public:
  benchmark(const std::string& output_dir) {
    output_dir_ = output_dir;

    struct stat st = { 0 };
    if (stat(output_dir.c_str(), &st) == -1)
      mkdir(output_dir.c_str(), 0777);
  }

 protected:
  template<typename op_t>
  void bench_thput(op_t&& op, const size_t nthreads,
                   const std::string& output_file) {
    std::vector<std::thread> workers(nthreads);
    std::vector<double> thput(nthreads);
    for (size_t i = 0; i < nthreads; i++) {
      workers[i] = std::thread(bench_thput_thread<data_structure, op_t>,
                               std::ref(op), std::ref(ds_), nthreads, i,
                               std::ref(thput));
      SET_CORE_AFFINITY(workers[i], i);
    }

    for (std::thread& worker : workers)
      worker.join();

    double thput_tot = std::accumulate(thput.begin(), thput.end(), 0.0);
    LOG_INFO<< "Completed benchmark at " << thput_tot << " ops/s";

    std::ofstream out(output_file);
    out << thput_tot << "\n";
    out.close();
  }

  data_structure ds_;
  std::string output_dir_;
};

}
}

#endif /* UTILS_BENCHMARK_H_ */

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
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <mutex>

#include "logger.h"

#define LOOP_OPS(op, i, ds, num_ops, limit) \
  start_time = time_utils::cur_s();\
  num_ops = 0;\
  while (time_utils::cur_s() - start_time < limit) {\
    op(i, ds);\
    num_ops++;\
  }

#define TIME_OPS(op, i, ds, num_ops, limit, out) \
  start_time = time_utils::cur_s();\
  num_ops = 0;\
  while (time_utils::cur_s() - start_time < limit) {\
    uint64_t ns = time_utils::time_function_ns(op, i, std::ref(ds));\
    out << ns << "\n";\
    num_ops++;\
  }\

#ifdef _GNU_SOURCE
#ifndef NPIN_CORES
#define SET_CORE_AFFINITY(t, core_id)\
  LOG_INFO << "Pinning thread to core" << core_id;\
  cpu_set_t cpuset;\
  CPU_ZERO(&cpuset);\
  CPU_SET(core_id, &cpuset);\
  int rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);\
  if (rc != 0)\
    LOG_WARN << "Error calling pthread_setaffinity_np: " << rc;
#else
#define SET_CORE_AFFINITY(thread, core_id)
#endif
#else
#ifndef NPIN_CORES
#define SET_CORE_AFFINITY(thread, core_id)\
  LOG_WARN << "Not pinning thread to core";
#else
#define SET_CORE_AFFINITY(thread, core_id)
#endif
#endif

#define BENCH_OP_THPUT(op, nthreads)\
  std::string output_file = this->output_dir_ + "/throughput-" + #op + "-"\
     + std::to_string(nthreads) + ".txt";\
  this->bench_thput(op, nthreads, output_file);

#define BENCH_OP_LATENCY(op)\
  std::string output_file = this->output_dir_ + "/latency-" + #op + ".txt";\
  this->bench_latency(op, output_file);

#define BENCH_OP_THPUT_BATCH(op, nthreads, batch_size)\
  std::string output_file = this->output_dir_ + "/throughput-" + #op + "-"\
     + std::to_string(nthreads) + "-" + std::to_string(batch_size) + ".txt";\
  this->bench_thput(op, nthreads, batch_size, output_file);

#define BENCH_OP_LATENCY_BATCH(op, batch_size)\
  std::string output_file = this->output_dir_ + "/latency-" + #op + "-"\
     + std::to_string(batch_size) + ".txt";\
  this->bench_latency(op, batch_size, output_file);

#define DEFINE_BENCH(op) DEFINE_BENCH_BATCH(op, 1)

#define DEFINE_BENCH_BATCH(op, batch_size)\
  void bench_throughput_##op(size_t nthreads) {\
    BENCH_OP_THPUT_BATCH(op, nthreads, batch_size)\
  }\
  void bench_latency_##op() {\
    BENCH_OP_LATENCY_BATCH(op, batch_size)\
  }

namespace utils {
namespace bench {

struct constants {
  static const uint64_t WARMUP_SECS = 30;
  static const uint64_t MEASURE_SECS = 60;
  static const uint64_t COOLDOWN_SECS = 30;
};

typedef std::chrono::high_resolution_clock timer;
typedef std::chrono::microseconds us;

template<typename data_structure, typename op_t>
static void bench_thput_thread(op_t&& op, data_structure& ds, size_t nthreads,
                               size_t batch_size, size_t i,
                               std::vector<double>& thput) {
  size_t num_ops;
  uint64_t start_time;

  LOG_INFO<< "Running warmup for " << constants::WARMUP_SECS << " s";
  LOOP_OPS(op, i, ds, num_ops, constants::WARMUP_SECS);

  LOG_INFO<< "Warmup complete; running measure for " << constants::MEASURE_SECS << " s";
  auto start = timer::now();
  LOOP_OPS(op, i, ds, num_ops, constants::MEASURE_SECS);
  auto end = timer::now();

  auto usecs = std::chrono::duration_cast<us>(end - start).count();
  assert(usecs > 0);
  thput[i] = num_ops * batch_size * 1e6 / usecs;

  LOG_INFO<< "Measure complete; running cooldown for " << constants::COOLDOWN_SECS << " s";
  LOOP_OPS(op, i, ds, num_ops, constants::COOLDOWN_SECS);

  LOG_INFO<< "Thread completed " << (thput[i] * usecs) / 1e6 << " ops at " << thput[i] << " ops/s";
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
  void bench_latency(op_t&& op, size_t batch_size,
                     const std::string& output_file) {
    size_t num_ops;
    uint64_t start_time;

    LOG_INFO<< "Running warmup for " << constants::WARMUP_SECS << " s";
    LOOP_OPS(op, 0, ds_, num_ops, constants::WARMUP_SECS);

    LOG_INFO<< "Warmup complete; running measure for " << constants::MEASURE_SECS << " s";
    std::ofstream out(output_file, std::ofstream::out | std::ofstream::app);
    TIME_OPS(op, 0, ds_, num_ops, constants::MEASURE_SECS, out);

    LOG_INFO<< "Measure complete; running cooldown for " << constants::COOLDOWN_SECS << " s";
    LOOP_OPS(op, 0, ds_, num_ops, constants::COOLDOWN_SECS);
  }

  template<typename op_t>
  void bench_thput(op_t&& op, const size_t nthreads, const size_t batch_size,
                   const std::string& output_file) {
    std::vector<std::thread> workers(nthreads);
    std::vector<double> thput(nthreads);
    for (size_t i = 0; i < nthreads; i++) {
      workers[i] = std::thread(bench_thput_thread<data_structure, op_t>,
                               std::ref(op), std::ref(ds_), nthreads,
                               batch_size, i, std::ref(thput));
      SET_CORE_AFFINITY(workers[i], i);
    }

    for (std::thread& worker : workers)
      worker.join();

    double thput_tot = std::accumulate(thput.begin(), thput.end(), 0.0);
    LOG_INFO<< "Completed benchmark at " << thput_tot << " ops/s";

    {
      std::lock_guard<std::mutex> lk(output_mtx_);
      std::ofstream out(output_file, std::ofstream::out | std::ofstream::app);
      out << thput_tot << "\n";
      out.close();
    }
  }

  data_structure ds_;
  std::mutex output_mtx_;
  std::string output_dir_;
};

}
}

#endif /* UTILS_BENCHMARK_H_ */

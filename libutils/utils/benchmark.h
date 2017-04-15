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
#include <string>

#include "logger.h"
#include "cmd_parse.h"

#define LOOP_OPS(op, i, ds, num_ops, time_limit, ops_limit) \
  start_time = time_utils::cur_s();\
  num_ops = 0;\
  while (time_utils::cur_s() - start_time < time_limit && num_ops < ops_limit) {\
    op(i, ds);\
    num_ops++;\
  }

#define TIME_OPS(op, i, ds, num_ops, time_limit, ops_limit, out) \
  start_time = time_utils::cur_s();\
  num_ops = 0;\
  while (time_utils::cur_s() - start_time < time_limit && num_ops < ops_limit) {\
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

static cmd_options benchmark_opts() {
  cmd_options opts;
  opts.add(
      cmd_option("warmup-secs", 'w', false).set_default("30").set_description(
          "Warmup time in seconds"));
  opts.add(
      cmd_option("measure-secs", 'm', false).set_default("60").set_description(
          "Measure time in seconds"));
  opts.add(
      cmd_option("measure-secs", 'c', false).set_default("60").set_description(
          "Measure time in seconds"));

  opts.add(
      cmd_option("warmup-count", 'W', false).set_default("500000")
          .set_description("#Warmup ops"));
  opts.add(
      cmd_option("measure-count", 'M', false).set_default("1000000")
          .set_description("#Measure ops"));
  opts.add(
      cmd_option("measure-count", 'C', false).set_default("5000000")
          .set_description("#Cooldown ops"));

  return opts;
}

struct benchmark_defaults {
  static const uint64_t WARMUP_SECS = 30;
  static const uint64_t MEASURE_SECS = 60;
  static const uint64_t COOLDOWN_SECS = 30;

  static const uint64_t WARMUP_COUNT = 500000;
  static const uint64_t MEASURE_COUNT = 1000000;
  static const uint64_t COOLDOWN_COUNT = 500000;
};

struct benchmark_limits {
  uint64_t warmup_secs;
  uint64_t measure_secs;
  uint64_t cooldown_secs;

  uint64_t warmup_count;
  uint64_t measure_count;
  uint64_t cooldown_count;

  benchmark_limits()
      : warmup_secs(benchmark_defaults::WARMUP_SECS),
        measure_secs(benchmark_defaults::MEASURE_SECS),
        cooldown_secs(benchmark_defaults::COOLDOWN_SECS),
        warmup_count(benchmark_defaults::WARMUP_COUNT),
        measure_count(benchmark_defaults::MEASURE_COUNT),
        cooldown_count(benchmark_defaults::COOLDOWN_COUNT) {
  }
};

static benchmark_limits parse_limits(const cmd_parser& parser) {
  benchmark_limits limits;
  limits.warmup_secs = parser.get_long("warmup-secs");
  limits.measure_secs = parser.get_long("measure-secs");
  limits.cooldown_secs = parser.get_long("cooldown-secs");
  limits.warmup_count = parser.get_long("warmup-count");
  limits.measure_count = parser.get_long("measure-count");
  limits.cooldown_count = parser.get_long("cooldown-count");
  return limits;
}

typedef std::chrono::high_resolution_clock timer;
typedef std::chrono::microseconds us;

template<typename data_structure, typename op_t>
static void bench_thput_thread(op_t&& op, data_structure& ds, size_t nthreads,
                               size_t batch_size, size_t i,
                               benchmark_limits& limits,
                               std::vector<double>& thput) {
  size_t num_ops;
  uint64_t start_time;

  LOG_INFO<< "Running warmup for min(" << limits.warmup_secs << "s, " << limits.warmup_count << "ops)";
  LOOP_OPS(op, i, ds, num_ops, limits.warmup_secs, limits.warmup_count);

  LOG_INFO<< "Warmup complete; running measure for min(" << limits.measure_secs << "s, " << limits.measure_count << "ops)";
  auto start = timer::now();
  LOOP_OPS(op, i, ds, num_ops, limits.measure_secs, limits.measure_count);
  auto end = timer::now();

  auto usecs = std::chrono::duration_cast<us>(end - start).count();
  assert(usecs > 0);
  thput[i] = num_ops * batch_size * 1e6 / usecs;

  LOG_INFO<< "Measure complete; running cooldown for min(" << limits.cooldown_secs << "s, " << limits.cooldown_count << "ops)";
  LOOP_OPS(op, i, ds, num_ops, limits.cooldown_secs, limits.cooldown_count);

  LOG_INFO<< "Thread completed " << (thput[i] * usecs) / 1e6 << " ops at " << thput[i] << " ops/s";
}

template<typename data_structure>
class benchmark {
 public:
  benchmark(const std::string& output_dir, const benchmark_limits& limits =
                benchmark_limits())
      : limits_(limits) {
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

    LOG_INFO<< "Running warmup for min(" << limits_.warmup_secs << "s, " << limits_.warmup_count << "ops)";
    LOOP_OPS(op, 0, ds_, num_ops, limits_.warmup_secs, limits_.warmup_count);

    LOG_INFO<< "Warmup complete; running measure for min(" << limits_.measure_secs << "s, " << limits_.measure_count << "ops)";
    std::ofstream out(output_file, std::ofstream::out | std::ofstream::app);
    TIME_OPS(op, 0, ds_, num_ops, limits_.measure_secs, limits_.measure_count,
             out);

    LOG_INFO<< "Measure complete; running cooldown for min(" << limits_.cooldown_secs << "s, " << limits_.cooldown_count << "ops)";
    LOOP_OPS(op, 0, ds_, num_ops, limits_.cooldown_secs, limits_.cooldown_count);
  }

  template<typename op_t>
  void bench_thput(op_t&& op, const size_t nthreads, const size_t batch_size,
                   const std::string& output_file) {
    std::vector<std::thread> workers(nthreads);
    std::vector<double> thput(nthreads);
    for (size_t i = 0; i < nthreads; i++) {
      workers[i] = std::thread(bench_thput_thread<data_structure, op_t>,
                               std::ref(op), std::ref(ds_), nthreads,
                               batch_size, i, std::ref(limits_),
                               std::ref(thput));
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
  benchmark_limits limits_;
};

}
}

#endif /* UTILS_BENCHMARK_H_ */

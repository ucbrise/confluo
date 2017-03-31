#include "concurrency_control.h"
#include "benchmark.h"
#include "cmd_parse.h"

using namespace ::datastore;

template<typename concurrency_control>
class concurrency_control_benchmark : public utils::bench::benchmark<
    concurrency_control> {
 public:
  concurrency_control_benchmark(const std::string& output_dir)
      : utils::bench::benchmark<concurrency_control>(output_dir) {
  }

  static void write(concurrency_control& cc) {
    stateful o;
    uint64_t id = cc.start_write_op();
    cc.init_object(o, id);
    cc.end_write_op(id);
  }

  DEFINE_BENCH(write)
};

int main(int argc, char** argv) {
  cmd_options opts;
  opts.add(
      cmd_option("bench-type", 'b', false).set_default("throughput-write")
          .set_description("Benchmark type"));
  opts.add(
      cmd_option("num-threads", 't', false).set_default("1").set_description(
          "Number of benchmark threads"));
  opts.add(
      cmd_option("output-dir", 'o', false).set_default("results")
          .set_description("Output directory"));
  opts.add(
      cmd_option("concurrency-control", 'c', false).set_default("read-stalled")
          .set_description("Concurrency control scheme"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int num_threads;
  std::string bench_type;
  std::string output_dir;
  std::string concurrency_control;
  try {
    bench_type = parser.get("bench-type");
    num_threads = parser.get_int("num-threads");
    output_dir = parser.get("output-dir");
    concurrency_control = parser.get("tail-scheme");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  fprintf(stderr, "bench_type=%s, num_threads=%d, output_dir=%s, cc=%s\n",
          bench_type.c_str(), num_threads, output_dir.c_str(),
          concurrency_control.c_str());

  if (concurrency_control == "write-stalled") {
    if (bench_type == "throughput-write") {
      concurrency_control_benchmark<datastore::write_stalled> perf(output_dir);
      perf.bench_throughput_write(num_threads);
    } else if (bench_type == "latency-write") {
      concurrency_control_benchmark<datastore::write_stalled> perf(output_dir);
      perf.bench_latency_write();
    }
  } else if (concurrency_control == "read-stalled") {
    if (bench_type == "throughput-write") {
      concurrency_control_benchmark<datastore::read_stalled> perf(output_dir);
      perf.bench_throughput_write(num_threads);
    } else if (bench_type == "latency-write") {
      concurrency_control_benchmark<datastore::read_stalled> perf(output_dir);
      perf.bench_latency_write();
    }
  } else {
    fprintf(stderr, "Unknown tail scheme: %s\n", concurrency_control.c_str());
  }

  return 0;
}

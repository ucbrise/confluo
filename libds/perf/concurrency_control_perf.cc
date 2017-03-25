#include "concurrency_control.h"
#include "benchmark.h"
#include "cmd_parse.h"

using namespace ::datastore;

template<typename concurrency_control>
class concurrency_control_benchmark : public utils::bench::benchmark<concurrency_control> {
 public:
  concurrency_control_benchmark(const std::string& output_dir)
      : utils::bench::benchmark<concurrency_control>(output_dir) {
  }

  static void write(concurrency_control& tail) {
    stateful o;
    uint64_t id = tail.start_write_op();
    tail.init_object(o);
    tail.end_write_op(id);
  }

  DEFINE_BENCH(write)
};

int main(int argc, char** argv) {
  cmd_options opts;
  opts.add(
      cmd_option("num-threads", 't', false).set_default("1").set_description(
          "Number of benchmark threads"));
  opts.add(
      cmd_option("output-dir", 'o', false).set_default("results")
          .set_description("Output directory"));
  opts.add(
      cmd_option("tail-scheme", 's', false).set_default("read-stalled")
          .set_description("Scheme for tail"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int num_threads;
  std::string output_dir;
  std::string concurrency_control;
  try {
    num_threads = parser.get_int("num-threads");
    output_dir = parser.get("output-dir");
    concurrency_control = parser.get("tail-scheme");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  fprintf(stderr, "num_threads=%d, output_dir=%s, concurrency_control=%s\n",
          num_threads, output_dir.c_str(), concurrency_control.c_str());

  if (concurrency_control == "write-stalled") {
    concurrency_control_benchmark<datastore::write_stalled> perf(output_dir);
    perf.bench_operation(num_threads);
  } else if (concurrency_control == "read-stalled") {
    concurrency_control_benchmark<datastore::read_stalled> perf(output_dir);
    perf.bench_operation(num_threads);
  } else {
    fprintf(stderr, "Unknown tail scheme: %s\n", concurrency_control.c_str());
  }

  return 0;
}

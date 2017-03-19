#include "tail_scheme.h"
#include "benchmark.h"
#include "cmd_parse.h"

using namespace ::datastore;

template<typename tail_scheme>
class tail_scheme_benchmark : public utils::bench::benchmark<tail_scheme> {
 public:
  tail_scheme_benchmark(const std::string& output_dir)
      : utils::bench::benchmark<tail_scheme>(output_dir) {
  }

  static void operation(tail_scheme& tail) {
    uint64_t id = tail.start_write_op();
    tail.end_write_op(id);
  }

  DEFINE_BENCH(operation)
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
  std::string tail_scheme;
  try {
    num_threads = parser.get_int("num-threads");
    output_dir = parser.get("output-dir");
    tail_scheme = parser.get("tail-scheme");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  fprintf(stderr, "num_threads=%d, output_dir=%s, tail_scheme=%s\n",
          num_threads, output_dir.c_str(), tail_scheme.c_str());

  if (tail_scheme == "write-stalled") {
    tail_scheme_benchmark<datastore::write_stalled_tail> perf(output_dir);
    perf.bench_operation(num_threads);
  } else if (tail_scheme == "read-stalled") {
    tail_scheme_benchmark<datastore::read_stalled_tail> perf(output_dir);
    perf.bench_operation(num_threads);
  } else {
    fprintf(stderr, "Unknown tail scheme: %s\n", tail_scheme.c_str());
  }

  return 0;
}

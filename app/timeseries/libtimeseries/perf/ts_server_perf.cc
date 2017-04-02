#include "client/client.h"
#define NPIN_CORES
#include "benchmark.h"
#include "logger.h"
#include "assertions.h"
#include "cmd_parse.h"
#include "time_utils.h"
#include "rand_utils.h"
#include "mmap_utils.h"

using namespace ::timeseries;

#define DATA_SIZE 64

class ts_server_benchmark : public utils::bench::benchmark<timeseries_db_client> {
 public:
  ts_server_benchmark(const std::string& output_dir,
                      const std::string& input_file, size_t load_records,
                      size_t batch_size, const std::string& host, int port)
      : utils::bench::benchmark<timeseries_db_client>(output_dir) {
    ds_.connect(host, port);
    PRELOAD_RECORDS = load_records;
    BATCH_SIZE = batch_size;
    BATCH_BYTES = BATCH_SIZE * sizeof(data_pt);
    cur_off = 0;
    data = (char*) utils::mmap_utils::mmap_r(input_file);
    for (size_t i = 0; i < load_records; i++) {
      ds_.insert_values(std::string(data + cur_off, BATCH_BYTES));
      cur_off += BATCH_BYTES;
    }
  }

  static void insert_values(size_t i, timeseries_db_client& client) {
    client.insert_values(std::string(data + cur_off, BATCH_BYTES));
    cur_off += BATCH_BYTES;
  }

  DEFINE_BENCH_BATCH(insert_values, BATCH_SIZE)

 private:
  static char* data;
  static size_t cur_off;
  static uint64_t PRELOAD_RECORDS;
  static uint64_t BATCH_SIZE;
  static size_t BATCH_BYTES;
};

uint64_t ts_server_benchmark::PRELOAD_RECORDS = 0;
uint64_t ts_server_benchmark::BATCH_SIZE = 1;
size_t ts_server_benchmark::BATCH_BYTES = sizeof(data_pt);
char* ts_server_benchmark::data;
size_t ts_server_benchmark::cur_off;

int main(int argc, char** argv) {
  cmd_options opts;
  opts.add(
      cmd_option("num-threads", 't', false).set_default("1").set_description(
          "Number of benchmark threads"));
  opts.add(
      cmd_option("input-file", 'i', false).set_default("data.txt")
          .set_description("Output directory"));
  opts.add(
      cmd_option("output-dir", 'o', false).set_default("results")
          .set_description("Output directory"));
  opts.add(
      cmd_option("bench-op", 'b', false).set_default("throughput-append")
          .set_description(
          "Benchmark operation (append, get, update, invalidate)"));
  opts.add(
      cmd_option("append-batchsize", 'a', false).set_default("1")
          .set_description("Append batch size"));
  opts.add(
      cmd_option("preload-records", 'r', false).set_default("0").set_description(
          "#Records to pre-load the logstore with"));
  opts.add(
      cmd_option("server", 's', false).set_default("localhost").set_description(
          "Server to connect"));
  opts.add(
      cmd_option("port", 'p', false).set_default("9090").set_description(
          "Server port"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int num_threads;
  std::string input_file;
  std::string output_dir;
  std::string bench_op;
  long load_records;
  long batch_size;
  std::string server;
  int port;
  try {
    num_threads = parser.get_int("num-threads");
    input_file = parser.get("input-file");
    output_dir = parser.get("output-dir");
    bench_op = parser.get("bench-op");
    load_records = parser.get_long("preload-records");
    batch_size = parser.get_long("append-batchsize");
    server = parser.get("server");
    port = parser.get_int("port");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  const char* fmt = "#threads=%d, ifile=%s, odir=%s, bench_op=%s, "
      "#records=%ld, batch-size=%ld, server=%s, port=%d\n";
  fprintf(stderr, fmt, num_threads, input_file.c_str(), output_dir.c_str(),
          bench_op.c_str(), load_records, batch_size, server.c_str(), port);

  ts_server_benchmark perf(output_dir, input_file, load_records, batch_size,
                           server, port);
  if (bench_op == "throughput-insert-values") {
    perf.bench_throughput_insert_values(num_threads);
  } else if (bench_op == "latency-insert-values") {
    perf.bench_latency_insert_values();
  } else {
    fprintf(stderr, "Unknown benchmark op: %s\n", bench_op.c_str());
  }

  return 0;
}


#include "client/client.h"
#define NPIN_CORES
#include "benchmark.h"
#include "logger.h"
#include "assertions.h"
#include "cmd_parse.h"
#include "time_utils.h"
#include "rand_utils.h"
#include "mmap_utils.h"
#include "error_handling.h"

using namespace ::timeseries;

#define DATA_SIZE 64

class ts_server_benchmark : public utils::bench::benchmark<timeseries_db_client> {
 public:
  ts_server_benchmark(const std::string& output_dir,
                      const std::string& input_file, size_t load_records,
                      size_t batch_size, const std::string& host, int port)
      : utils::bench::benchmark<timeseries_db_client>(output_dir) {
    ds_.connect(host, port);
    BATCH_SIZE = batch_size;
    BATCH_BYTES = BATCH_SIZE * sizeof(data_pt);
    cur_off = 0;
    data_count = utils::mmap_utils::file_size(input_file) / sizeof(data_pt);
    data = (char*) utils::mmap_utils::mmap_r(input_file);

    LOG_INFO<< "Pre-loading " << load_records << " data points...";
    uint64_t preload_batch_bytes = 8192 * sizeof(data_pt);
    for (size_t i = 0; i < load_records / 8192; i++) {
      ds_.insert_values(std::string(data + cur_off, preload_batch_bytes));
      cur_off += preload_batch_bytes;
    }
    std::string final_batch = std::string(
        data + cur_off, load_records * sizeof(data_pt) % preload_batch_bytes);
    if (!final_batch.empty()) {
      ds_.insert_values(std::string(data + cur_off, final_batch.length()));
      cur_off += final_batch.length();
    }
    PRELOAD_RECORDS = ds_.num_entries();
    LOG_INFO<< "Pre-load complete, loaded " << PRELOAD_RECORDS << " data points";
  }

  static void insert_values(size_t i, timeseries_db_client& client) {
    client.insert_values(std::string(data + cur_off, BATCH_BYTES));
    cur_off += BATCH_BYTES;
  }

  static void get_range(size_t i, timeseries_db_client& client) {
    data_pt* first = (data_pt*) (data
        + utils::rand_utils::rand_uint64(PRELOAD_RECORDS - BATCH_SIZE)
        * sizeof(data_pt));
    data_pt* last = first + BATCH_SIZE - 1;
    std::string res;
    client.get_range_latest(res, first->timestamp, last->timestamp);
  }

  DEFINE_BENCH_BATCH(insert_values, BATCH_SIZE)
  DEFINE_BENCH_BATCH(get_range, BATCH_SIZE)

private:
  static char* data;
  static size_t data_count;
  static size_t cur_off;
  static uint64_t PRELOAD_RECORDS;
  static uint64_t BATCH_SIZE;
  static size_t BATCH_BYTES;
};

uint64_t ts_server_benchmark::PRELOAD_RECORDS = 0;
uint64_t ts_server_benchmark::BATCH_SIZE = 1;
size_t ts_server_benchmark::BATCH_BYTES = sizeof(data_pt);
char* ts_server_benchmark::data;
size_t ts_server_benchmark::data_count;
size_t ts_server_benchmark::cur_off;

int main(int argc, char** argv) {
  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);

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
      cmd_option("batch-size", 'B', false).set_default("1")
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
    batch_size = parser.get_long("batch-size");
    server = parser.get("server");
    port = parser.get_int("port");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  LOG_INFO << parser.parsed_values();

  ts_server_benchmark perf(output_dir, input_file, load_records, batch_size,
                           server, port);
  if (bench_op == "throughput-insert-values") {
    perf.bench_throughput_insert_values(num_threads);
  } else if (bench_op == "throughput-get-range") {
    perf.bench_throughput_get_range(num_threads);
  } else if (bench_op == "latency-insert-values") {
    perf.bench_latency_insert_values();
  } else if (bench_op == "latency-get-range") {
    perf.bench_latency_get_range();
  } else {
    fprintf(stderr, "Unknown benchmark op: %s\n", bench_op.c_str());
  }

  return 0;
}


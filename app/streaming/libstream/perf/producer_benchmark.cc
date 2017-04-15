#include "producer.h"
#define NPIN_CORES
#include "benchmark.h"
#include "logger.h"
#include "assertions.h"
#include "cmd_parse.h"
#include "time_utils.h"
#include "rand_utils.h"
#include "error_handling.h"

using namespace ::streaming;

typedef streaming::uuid_t stream_id_t;

class producer_benchmark : public utils::bench::benchmark<producer> {
 public:
  producer_benchmark(const std::string& output_dir, stream_id_t id,
                     size_t batch_size, size_t record_size,
                     const std::string& host, int port)
      : utils::bench::benchmark<producer>(output_dir) {

    ds_.connect(host, port);
    ds_.set_uuid(id);

    BATCH_SIZE = batch_size;
    DATA_SIZE = record_size;
    std::string record(DATA_SIZE, 'x');
    assert_throw(
        record.length() == DATA_SIZE,
        "Record size = " << record.length() << " DATA_SIZE = " << DATA_SIZE);
    APPEND_DATA_BATCH.resize(BATCH_SIZE, record);
  }

  static void send(size_t i, producer& p) {
    p.send(APPEND_DATA_BATCH);
  }

  DEFINE_BENCH_BATCH(send, BATCH_SIZE)

 private:
  static uint64_t BATCH_SIZE;
  static size_t DATA_SIZE;
  static std::vector<std::string> APPEND_DATA_BATCH;
};

uint64_t producer_benchmark::BATCH_SIZE;
size_t producer_benchmark::DATA_SIZE;
std::vector<std::string> producer_benchmark::APPEND_DATA_BATCH;

int main(int argc, char** argv) {
  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);

  cmd_options opts;
  opts.add(
      cmd_option("output-dir", 'o', false).set_default("results")
          .set_description("Output directory"));
  opts.add(
      cmd_option("type", 't', false).set_default("latency").set_description(
          "Benchmark type (latency, throughput)"));
  opts.add(
      cmd_option("stream-id", 's', false).set_default("1").set_description(
          "Stream ID"));
  opts.add(
      cmd_option("data-size", 'd', false).set_default("64").set_description(
          "Data record size"));
  opts.add(
      cmd_option("batch-size", 'B', false).set_default("1").set_description(
          "Append batch size"));
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

  std::string output_dir;

  long uuid;
  long batch_size;
  long record_size;
  std::string type;
  std::string server;
  int port;
  try {
    output_dir = parser.get("output-dir");
    type = parser.get("type");
    uuid = parser.get_long("stream-id");
    batch_size = parser.get_long("batch-size");
    record_size = parser.get_long("data-size");
    server = parser.get("server");
    port = parser.get_int("port");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  LOG_INFO<< parser.parsed_values();

  producer_benchmark perf(output_dir, uuid, batch_size, record_size, server,
                          port);

  if (type == "throughput") {
    perf.bench_throughput_send(1);
  } else if (type == "latency") {
    perf.bench_latency_send();
  } else {
    fprintf(stderr, "Unrecognized type: %s\n", type.c_str());
    return -1;
  }

  return 0;
}


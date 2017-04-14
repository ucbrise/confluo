#include "consumer.h"
#define NPIN_CORES
#include "benchmark.h"
#include "logger.h"
#include "assertions.h"
#include "cmd_parse.h"
#include "time_utils.h"
#include "rand_utils.h"

using namespace ::streaming;

typedef streaming::uuid_t stream_id_t;

class consumer_benchmark : public utils::bench::benchmark<consumer> {
 public:
  consumer_benchmark(const std::string& output_dir, stream_id_t id,
                     size_t batch_size, size_t record_size,
                     const std::string& host, int port)
      : utils::bench::benchmark<consumer>(output_dir) {

    ds_.connect(host, port);
    ds_.set_uuid(id);
    BATCH_SIZE = batch_size;
    DATA_SIZE = record_size;
    BATCH_BYTES = BATCH_SIZE * (DATA_SIZE + sizeof(uint32_t));
    ds_.set_batch_bytes(BATCH_BYTES);
  }

  static void recv(size_t i, consumer& p) {
    std::vector<std::string> batch;
    p.recv(batch);
  }

  DEFINE_BENCH_BATCH(recv, BATCH_SIZE)

 private:
  static uint64_t BATCH_SIZE;
  static size_t DATA_SIZE;
  static size_t BATCH_BYTES;
};

uint64_t consumer_benchmark::BATCH_SIZE;
size_t consumer_benchmark::DATA_SIZE;
size_t consumer_benchmark::BATCH_BYTES;

int main(int argc, char** argv) {
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
      cmd_option("num-threads", 't', false).set_default("1").set_description(
          "Number of benchmark threads"));
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

  consumer_benchmark perf(output_dir, uuid, batch_size, record_size, server,
                          port);

  if (type == "throughput") {
    perf.bench_throughput_recv(1);
  } else if (type == "latency") {
    perf.bench_latency_recv();
  } else {
    fprintf(stderr, "Unrecognized type: %s\n", type.c_str());
    return -1;
  }

  return 0;
}


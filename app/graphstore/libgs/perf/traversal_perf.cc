#include "client/traversal_client.h"
#define NPIN_CORES
#include "benchmark.h"
#include "logger.h"
#include "assertions.h"
#include "cmd_parse.h"
#include "time_utils.h"
#include "rand_utils.h"
#include "error_handling.h"

using namespace ::graphstore;
using namespace ::utils::bench;

class traversal_benchmark : public utils::bench::benchmark<traversal_client> {
 public:
  traversal_benchmark(const std::string& output_dir,
                      const benchmark_limits& limits, const int64_t node_id,
                      const int64_t link_type, const int64_t depth,
                      const int64_t breadth,
                      const std::vector<std::string>& hosts, const int port)
      : utils::bench::benchmark<traversal_client>(output_dir, limits) {
    NODE_ID = node_id;
    LINK_TYPE = link_type;
    DEPTH = depth;
    BREADTH = breadth;
    ds_.setup(hosts, port, 0);
    LOG_INFO<< "Traversal benchmark setup complete.";
  }

  static void traversal(size_t i, traversal_client& client) {
    std::vector<TLink> results;
    LOG_INFO << "Issuing traversal query...";
    client.traverse(results, NODE_ID++, LINK_TYPE, DEPTH, BREADTH);
    LOG_INFO << "Traversal query completed, found " << results.size() << " results";
  }

  DEFINE_BENCH(traversal)

private:
  static int64_t NODE_ID;
  static int64_t LINK_TYPE;
  static int64_t DEPTH;
  static int64_t BREADTH;
};

int64_t traversal_benchmark::NODE_ID;
int64_t traversal_benchmark::LINK_TYPE;
int64_t traversal_benchmark::DEPTH;
int64_t traversal_benchmark::BREADTH;

std::vector<std::string> read_hosts(const std::string& hostfile) {
  std::vector<std::string> hostlist;
  std::ifstream in(hostfile);
  std::copy(std::istream_iterator<std::string>(in),
            std::istream_iterator<std::string>(), std::back_inserter(hostlist));
  return hostlist;
}

int main(int argc, char** argv) {
  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);

  cmd_options opts = utils::bench::benchmark_opts();
  opts.add(
      cmd_option("output-dir", 'o', false).set_default("results")
          .set_description("Output directory"));
  opts.add(
      cmd_option("type", 't', false).set_default("latency").set_description(
          "Benchmark type (latency, throughput)"));
  opts.add(
      cmd_option("node-id", 'n', false).set_default("1").set_description(
          "Node ID to begin with"));
  opts.add(
      cmd_option("link-type", 'l', false).set_default("0").set_description(
          "Traversal is confined to the specified link type"));
  opts.add(
      cmd_option("depth", 'd', false).set_default("5").set_description(
          "Traversal is confined to the specified depth"));
  opts.add(
      cmd_option("breadth", 'b', false).set_default("64").set_description(
          "Traversal is confined to the specified breadth"));
  opts.add(
      cmd_option("host-list", 'H', false).set_default("conf/hosts")
          .set_description("File containing list of graph store servers"));
  opts.add(
      cmd_option("port", 'p', false).set_default("9090").set_description(
          "Server port"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  std::string output_dir;

  int64_t node_id;
  int64_t link_type;
  int64_t depth;
  int64_t breadth;
  std::string type;
  std::string hosts_file;
  int port;
  benchmark_limits limits;
  try {
    output_dir = parser.get("output-dir");
    type = parser.get("type");
    node_id = parser.get_long("node-id");
    link_type = parser.get_long("link-type");
    depth = parser.get_long("depth");
    breadth = parser.get_long("breadth");
    hosts_file = parser.get("host-list");
    port = parser.get_int("port");
    limits = utils::bench::parse_limits(parser);
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  LOG_INFO<< parser.parsed_values();

  traversal_benchmark perf(output_dir, limits, node_id, link_type, depth,
                           breadth, read_hosts(hosts_file), port);

  if (type == "throughput") {
    perf.bench_throughput_traversal(1);
  } else if (type == "latency") {
    perf.bench_latency_traversal();
  } else {
    fprintf(stderr, "Unrecognized type: %s\n", type.c_str());
    return -1;
  }

  return 0;
}


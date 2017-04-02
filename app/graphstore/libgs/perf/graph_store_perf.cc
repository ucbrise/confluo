#include "graph_store.h"

#include <chrono>

#include "benchmark.h"
#include "cmd_parse.h"
#include "logger.h"
#include "rand_utils.h"

using namespace graphstore;
using namespace utils::log;

#define PREAMBLE_RNG\
  static thread_local std::mt19937 generator;\
  std::uniform_int_distribution<int64_t> distribution(0, INIT_NODES)

template<typename tail_scheme>
class graph_store_perf :
    public utils::bench::benchmark<graph_store<tail_scheme>> {
 public:
  static const uint64_t NODE_TYPE = 1;
  static const uint64_t NUM_LINK_TYPES = 2;
  static const uint64_t INIT_NODES = 1000000;
  static const uint64_t INIT_DEGREE = 80;
  static const std::string DATA;

  graph_store_perf(const std::string& output_dir)
      : utils::bench::benchmark<graph_store<tail_scheme>>(output_dir) {
    // Pre-load data
    LOG_INFO<< "Loading nodes...";
    node_op op = create_node_op();
    for (size_t i = 0; i < INIT_NODES; i++) {
      this->ds_.add_node(op);
    }
    LOG_INFO<< "Finished loading " << INIT_NODES << " nodes";

    LOG_INFO<< "Loading links...";
    uint64_t num_links = 0;
    for (uint64_t id1 = 0; id1 < INIT_NODES; id1++) {
      size_t degree = utils::rand_utils::rand_int64(INIT_DEGREE);
      for (size_t j = 0; j < degree; j++) {
        int64_t link_type = utils::rand_utils::rand_int64(NUM_LINK_TYPES);
        int64_t id2 = utils::rand_utils::rand_int64(INIT_NODES);
        link_op op = create_link_op(id1, link_type, id2);
        this->ds_.add_link(op);
        num_links++;
        if (num_links % INIT_NODES == 0)
        LOG_INFO<< "Finished loading " << num_links << " links";
      }
    }
    LOG_INFO<< "Finished loading " << num_links << " links";
  }

  static void add_node(size_t i, graph_store<tail_scheme>& gs) {
    node_op op = graph_store_perf::create_node_op();
    gs.add_node(op);
  }

  static void update_node(size_t i, graph_store<tail_scheme>& gs) {
    PREAMBLE_RNG;
    int64_t id = distribution(generator);
    node_op op = create_node_op(id);
    gs.update_node(op);
  }

  static void delete_node(size_t i, graph_store<tail_scheme>& gs) {
    PREAMBLE_RNG;
    int64_t id = distribution(generator);
    gs.delete_node(NODE_TYPE, id);
  };

  static void get_node(size_t i, graph_store<tail_scheme>& gs) {
    PREAMBLE_RNG;
    int64_t id = distribution(generator);
    gs.get_node(NODE_TYPE, id);
  };

  static void add_link(size_t i, graph_store<tail_scheme>& gs) {
    PREAMBLE_RNG;
    int64_t id1 = distribution(generator);
    int64_t id2 = distribution(generator);
    link_op op = create_link_op(id1, 0, id2);
    gs.add_link(op);
  };

  static void update_link(size_t i, graph_store<tail_scheme>& gs) {
    PREAMBLE_RNG;
    int64_t id1 = distribution(generator);
    int64_t id2 = distribution(generator);
    link_op op = create_link_op(id1, 0, id2);
    gs.add_link(op);
  };

  static void delete_link(size_t i, graph_store<tail_scheme>& gs) {
    PREAMBLE_RNG;
    int64_t id1 = distribution(generator);
    int64_t id2 = distribution(generator);
    gs.delete_link(id1, 0, id2);
  };

  static void get_link(size_t i, graph_store<tail_scheme>& gs) {
    PREAMBLE_RNG;
    int64_t id1 = distribution(generator);
    int64_t id2 = distribution(generator);
    gs.get_link(id1, 0, id2);
  };

  static void get_link_list(size_t i, graph_store<tail_scheme>& gs) {
    PREAMBLE_RNG;
    int64_t id1 = distribution(generator);
    gs.get_link_list(id1, 0);
  };

  static void count_links(size_t i, graph_store<tail_scheme>& gs) {
    PREAMBLE_RNG;
    int64_t id1 = distribution(generator);
    gs.count_links(id1, 0);
  };

  static node_op create_node_op(uint64_t id = 0) {
    node_op op;
    op.data = DATA;
    op.type = NODE_TYPE;
    op.id = id;
    return op;
  }

  static link_op create_link_op(int64_t id1, int64_t link_type, int64_t id2) {
    link_op op;
    op.data = DATA;
    op.id1 = id1;
    op.link_type = link_type;
    op.id2 = id2;
    op.time = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    return op;
  }

  DEFINE_BENCH(add_node)

  DEFINE_BENCH(update_node)

  DEFINE_BENCH(delete_node)

  DEFINE_BENCH(get_node)

  DEFINE_BENCH(add_link)

  DEFINE_BENCH(update_link)

  DEFINE_BENCH(delete_link)

  DEFINE_BENCH(get_link)

  DEFINE_BENCH(get_link_list)

  DEFINE_BENCH(count_links)
};

template<typename tail_scheme>
const std::string graph_store_perf<tail_scheme>::DATA =
    "123random123alpha-numeric123text123random123alpha-numeric123text123random123alpha-numeric123text123random123alpha-numeric123text";

template<typename tail_scheme>
void exec_bench(graph_store_perf<tail_scheme>& perf, int num_threads,
                const std::string& bench_type) {
  if (bench_type == "add_node")
    perf.bench_throughput_add_node(num_threads);
  else if (bench_type == "update_node")
    perf.bench_throughput_update_node(num_threads);
  else if (bench_type == "delete_node")
    perf.bench_throughput_delete_node(num_threads);
  else if (bench_type == "get_node")
    perf.bench_throughput_get_node(num_threads);
  else if (bench_type == "add_link")
    perf.bench_throughput_add_link(num_threads);
  else if (bench_type == "update_link")
    perf.bench_throughput_update_link(num_threads);
  else if (bench_type == "delete_link")
    perf.bench_throughput_delete_link(num_threads);
  else if (bench_type == "get_link")
    perf.bench_throughput_get_link(num_threads);
  else if (bench_type == "get_link_list")
    perf.bench_throughput_get_link_list(num_threads);
  else if (bench_type == "count_links")
    perf.bench_throughput_count_links(num_threads);
  else
    fprintf(stderr, "Unknown benchmark type: %s\n", bench_type.c_str());
}

int main(int argc, char** argv) {
  cmd_options opts;
  opts.add(
      cmd_option("num-threads", 't', false).set_default("1").set_description(
          "Number of benchmark threads"));
  opts.add(
      cmd_option("bench-type", 'b', false).set_default("get_node")
          .set_description("Benchmark type"));
  opts.add(
      cmd_option("output-dir", 'o', false).set_default("results")
          .set_description("Output directory"));
  opts.add(
      cmd_option("tail-scheme", 's', false).set_default("read-stalled")
          .set_description("Scheme for graph tail"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int num_threads;
  std::string bench_type;
  std::string output_dir;
  std::string tail_scheme;
  try {
    num_threads = parser.get_int("num-threads");
    bench_type = parser.get("bench-type");
    output_dir = parser.get("output-dir");
    tail_scheme = parser.get("tail-scheme");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  fprintf(stderr,
          "bench_type=%s, num_threads=%d, output_dir=%s, tail_scheme=%s\n",
          bench_type.c_str(), num_threads, output_dir.c_str(),
          tail_scheme.c_str());

  if (tail_scheme == "write-stalled") {
    graph_store_perf<datastore::write_stalled> perf(output_dir);
    exec_bench(perf, num_threads, bench_type);
  } else if (tail_scheme == "read-stalled") {
    graph_store_perf<datastore::read_stalled> perf(output_dir);
    exec_bench(perf, num_threads, bench_type);
  } else {
    fprintf(stderr, "Unknown tail scheme: %s\n", tail_scheme.c_str());
  }

  return 0;
}

#include "micro_benchmark.h"

#include <chrono>
#include <ctime>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>
#include <condition_variable>
#include <unistd.h>
#include <sstream>
#include <algorithm>
#include <unistd.h>

#ifdef NO_LOG
#define LOG(out, fmt, ...)
#else
#define LOG(out, fmt, ...) fprintf(out, fmt, ##__VA_ARGS__)
#endif

#define QUERY(i, num_keys)
//#define QUERY(i, num_keys) {\
//  std::set<int64_t> search_res;\
//  if (query_types[i % kThreadQueryCount] == 0) {\
//    shard_->get(get_res, keys[i % keys.size()]);\
//    num_keys++;\
//  } else if (query_types[i % kThreadQueryCount] == 1) {\
//    shard_->search(search_res, terms[i % terms.size()]);\
//    num_keys += search_res.size();\
//  } else if (query_types[i % kThreadQueryCount] == 2) {\
//    shard_->insert(cur_key, values[i % values.size()]);\
//    num_keys++;\
//  }\
//}

micro_benchmark::micro_benchmark(std::string& data_path) {
  shard_ = new log_store<UINT_MAX>();
  char resolved_path[100];
  realpath(data_path.c_str(), resolved_path);
  data_path_ = resolved_path;

  /** Load the data
   *  =============
   *
   *  We load the log store with ~250MB of data (~25% of total capacity).
   */

  const int64_t target_data_size = 250 * 1024 * 1024;
  int64_t load_data_size = 0;
  load_end_offset_ = 0;
  load_keys_ = 0;
}

void micro_benchmark::benchmark_get_latency() {
  // Generate queries
  LOG(stderr, "Generating queries...");
  std::vector<int64_t> keys;

  for (int64_t i = 0; i < kWarmupCount + kMeasureCount; i++) {
    int64_t key = rand() % load_keys_;
    keys.push_back(key);
  }

  LOG(stderr, "Done.\n");

  std::ofstream result_stream("latency_get");

  unsigned char result[10 * 1024];
  // Warmup
  LOG(stderr, "Warming up for %llu queries...\n", kWarmupCount);
  for (uint64_t i = 0; i < kWarmupCount; i++) {
    shard_->get(result, keys[i]);
  }
  LOG(stderr, "Warmup complete.\n");

  // Measure
  LOG(stderr, "Measuring for %llu queries...\n", kMeasureCount);
  for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
    auto t0 = high_resolution_clock::now();
    shard_->get(result, keys[i]);
    auto t1 = high_resolution_clock::now();
    auto tdiff = duration_cast<nanoseconds>(t1 - t0).count();
    result_stream << keys[i] << "\t" << tdiff << "\n";
  }
  LOG(stderr, "Measure complete.\n");
  result_stream.close();
}

void micro_benchmark::benchmark_filter_latency() {
  LOG(stderr, "Reading queries...");
  std::vector<std::string> queries;

  std::ifstream in(data_path_ + ".queries");
  std::string query;
  while (queries.size() < kWarmupCount + kMeasureCount
      && std::getline(in, query)) {
    queries.push_back(query);
  }

  size_t warmup_count = queries.size() / 10;
  size_t measure_count = queries.size() - warmup_count;
  LOG(stderr, "Done.\n");

  std::ofstream result_stream("latency_search");

  // Warmup
  LOG(stderr, "Warming up for %llu queries...\n", warmup_count);
  for (uint64_t i = 0; i < warmup_count; i++) {
    std::string query = queries[i % queries.size()];
    std::set<int64_t> results;
    // shard_->filter(results, query);
  }
  LOG(stderr, "Warmup complete.\n");

  // Measure
  LOG(stderr, "Measuring for %llu queries...\n", measure_count);
  for (uint64_t i = warmup_count; i < warmup_count + measure_count; i++) {
    std::string query = queries[i % queries.size()];
    std::set<int64_t> results;
    auto t0 = high_resolution_clock::now();
    // shard_->search(results, query);
    auto t1 = high_resolution_clock::now();
    auto tdiff = duration_cast<nanoseconds>(t1 - t0).count();
    result_stream << results.size() << "\t" << tdiff << "\n";
  }
  LOG(stderr, "Measure complete.\n");
  result_stream.close();
}

void micro_benchmark::benchmark_insert_latency() {
  // Generate queries
  LOG(stderr, "Generating queries...");
  std::vector<std::string> values;

  std::ifstream in(data_path_ + ".inserts");
  for (int64_t i = 0; i < kWarmupCount + kMeasureCount; i++) {
    std::string cur_value;
    std::getline(in, cur_value);
    values.push_back(cur_value);
  }
  int64_t cur_key = load_keys_;

  LOG(stderr, "Done.\n");

  std::ofstream result_stream("latency_append");

  // Warmup
  LOG(stderr, "Warming up for %llu queries...\n", kWarmupCount);
  for (uint64_t i = 0; i < kWarmupCount; i++) {
    std::string cur_value = values[i];
    // shard_->insert(cur_key++, cur_value);
  }
  LOG(stderr, "Warmup complete.\n");

  // Measure
  LOG(stderr, "Measuring for %llu queries...\n", kMeasureCount);
  for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
    std::string cur_value = values[i];
    auto t0 = high_resolution_clock::now();
    // shard_->insert(cur_key++, cur_value);
    auto t1 = high_resolution_clock::now();
    auto tdiff = duration_cast<nanoseconds>(t1 - t0).count();
    result_stream << (cur_key - 1) << "\t" << tdiff << "\n";
  }
  LOG(stderr, "Measure complete.\n");
  result_stream.close();
}

void micro_benchmark::benchmark_throughput(const double get_f,
                                           const double search_f,
                                           const double append_f,
                                           const uint32_t num_clients) {

  if (get_f + search_f + append_f != 1.0) {
    LOG(stderr, "Query fractions must add up to 1.0. Sum = %lf\n",
        get_f + search_f + append_f);
    return;
  }

  const double get_m = get_f, search_m = get_f + search_f, append_m = get_f
      + search_f + append_f;

  std::condition_variable cvar;
  std::vector<std::thread> threads;

  for (uint32_t i = 0; i < num_clients; i++) {
    threads.push_back(
        std::move(
            std::thread([&] {
              std::vector<int64_t> keys;
              std::vector<std::string> values, terms;

              std::ifstream in_s(data_path_ + ".queries");
              std::ifstream in_a(data_path_ + ".inserts");
              int64_t cur_key = load_keys_;
              std::string term, value;
              std::vector<uint32_t> query_types;
              LOG(stderr, "Generating queries...\n");
              for (int64_t i = 0; i < kThreadQueryCount; i++) {
                int64_t key = random_integer(0, load_keys_);

                keys.push_back(key);
                if (std::getline(in_s, term)) terms.push_back(term);
                if (std::getline(in_a, value)) values.push_back(value);

                double r = random_double(0, 1);
                if (r <= get_m) {
                  query_types.push_back(0);
                } else if (r <= search_m) {
                  query_types.push_back(1);
                } else if (r <= append_m) {
                  query_types.push_back(2);
                }
              }

              std::shuffle(keys.begin(), keys.end(), prng());
              std::shuffle(terms.begin(), terms.end(), prng());
              std::shuffle(values.begin(), values.end(), prng());
              LOG(stderr, "Done.\n");

              double query_thput = 0;
              double key_thput = 0;
              char get_res[10*1024];

              try {
                // Warmup phase
                long i = 0;
                long num_keys = 0;
                timestamp_t warmup_start = get_timestamp();
                while (get_timestamp() - warmup_start < kWarmupTime) {
                  QUERY(i, num_keys);
                  i++;
                }

                // Measure phase
                i = 0;
                num_keys = 0;
                timestamp_t start = get_timestamp();
                while (get_timestamp() - start < kMeasureTime) {
                  QUERY(i, num_keys);
                  i++;
                }
                timestamp_t end = get_timestamp();
                double totsecs = (double) (end - start) / (1000.0 * 1000.0);
                query_thput = ((double) i / totsecs);
                key_thput = ((double) num_keys / totsecs);

                // Cooldown phase
                i = 0;
                num_keys = 0;
                timestamp_t cooldown_start = get_timestamp();
                while (get_timestamp() - cooldown_start < kCooldownTime) {
                  QUERY(i, num_keys);
                  i++;
                }

              } catch (std::exception &e) {
                LOG(stderr, "Throughput thread ended prematurely.\n");
              }

              LOG(stderr, "Throughput: %lf\n", query_thput);

              std::ofstream ofs;
              char output_file[100];
              sprintf(output_file, "throughput_%.2f_%.2f_%.2f_%.2f_%d", get_f, search_f, append_f, num_clients);
              ofs.open(output_file, std::ofstream::out | std::ofstream::app);
              ofs << query_thput << "\t" << key_thput << "\n";
              ofs.close();

            })));
  }

#ifdef MEASURE_GAP
  OPEN_GAP_LOG(get_f, search_f, append_f, delete_f, num_clients);
  timestamp_t start = get_timestamp();
  while (get_timestamp() - start < kWarmupTime + kMeasureTime + kCooldownTime) {
    LOG_GAP;
  }
#endif

  for (auto& th : threads) {
    th.join();
  }

}

void PrintUsage(char *exec) {
  LOG(stderr,
      "Usage: %s [-b bench-type] [-m mode] [-n num-clients] data-path\n", exec);
}

std::vector<std::string> &Split(const std::string &s, char delim,
                                std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::vector<std::string> Split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  Split(s, delim, elems);
  return elems;
}

int main(int argc, char** argv) {
  if (argc < 2 || argc > 6) {
    PrintUsage(argv[0]);
    return -1;
  }

  int c;
  std::string bench_type = "latency-get";
  int num_clients = 1;
  while ((c = getopt(argc, argv, "b:n:")) != -1) {
    switch (c) {
      case 'b':
        bench_type = std::string(optarg);
        break;
      case 'n':
        num_clients = atoi(optarg);
        break;
      default:
        LOG(stderr, "Could not parse command line arguments.\n");
    }
  }

  if (optind == argc) {
    PrintUsage(argv[0]);
    return -1;
  }

  std::string data_path = std::string(argv[optind]);

  micro_benchmark ls_bench(data_path);
  if (bench_type == "latency-get") {
    ls_bench.benchmark_get_latency();
  } else if (bench_type == "latency-filter") {
    ls_bench.benchmark_filter_latency();
  } else if (bench_type == "latency-insert") {
    ls_bench.benchmark_insert_latency();
  } else if (bench_type.find("throughput") == 0) {
    std::vector<std::string> tokens = Split(bench_type, '-');
    if (tokens.size() != 4) {
      LOG(stderr, "Error: Incorrect throughput benchmark format.\n");
      return -1;
    }
    double get_f = atof(tokens[1].c_str());
    double search_f = atof(tokens[2].c_str());
    double append_f = atof(tokens[3].c_str());
    LOG(stderr,
        "get_f = %.2lf, search_f = %.2lf, append_f = %.2lf, num_clients = %d\n",
        get_f, search_f, append_f, num_clients);
    ls_bench.benchmark_throughput(get_f, search_f, append_f, num_clients);
  } else {
    LOG(stderr, "Unknown benchmark type: %s\n", bench_type.c_str());
  }

  return 0;
}

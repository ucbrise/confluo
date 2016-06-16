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
#include "profiler.h"

#ifdef NO_LOG
#define LOG(out, fmt, ...)
#else
#define LOG(out, fmt, ...) fprintf(out, fmt, ##__VA_ARGS__)
#endif

#define MEASURE_GAP

#ifdef MEASURE_GAP
#define OPEN_GAP_LOG(get_f, search_f, append_f, delete_f, num_clients)\
  char gap_log_fname[100];\
  sprintf(gap_log_fname, "gap_%.2lf_%.2lf_%.2lf_%.2lf_%u.log", get_f, search_f, append_f, delete_f, num_clients);\
  std::ofstream gap(gap_log_fname);\
  uint64_t last_log_time;
#define LOG_GAP(last_log_time)\
  if (GetTimestamp() - last_log_time >= 500000) {\
    uint64_t g = shard_->GetGap();\
    uint64_t ts = GetTimestamp();\
    gap << ts << "\t" << g & 0xFFFFFFFF << "\t" << (g >> 32) << "\n";\
    last_log_time = ts;\
  }
#else
#define OPEN_GAP_LOG(get_f, search_f, append_f, delete_f, num_clients)
#define LOG_GAP(last_log_time)
#endif

#define QUERY(i, num_keys) {\
  std::set<int64_t> search_res;\
  if (query_types[i % kThreadQueryCount] == 0) {\
    shard_->Get(get_res, keys[i % keys.size()]);\
    num_keys++;\
  } else if (query_types[i % kThreadQueryCount] == 1) {\
    shard_->Search(search_res, terms[i % terms.size()]);\
    num_keys += search_res.size();\
  } else if (query_types[i % kThreadQueryCount] == 2) {\
    shard_->Append(cur_key, values[i % values.size()]);\
    num_keys++;\
  } else {\
    shard_->Delete(keys[i % keys.size()]);\
    num_keys++;\
  }\
}

MicroBenchmark::MicroBenchmark(std::string& data_path, int mode, bool dump) {
  shard_ = new LogStore<>();
  char resolved_path[100];
  realpath(data_path.c_str(), resolved_path);
  data_path_ = resolved_path;

  if (mode == 0) {
    /** Load the data
     *  =============
     *
     *  We load the log store with ~250MB of data (~25% of total capacity).
     */

    const int64_t target_data_size = 250 * 1024 * 1024;
    int64_t load_data_size = 0;
    load_end_offset_ = 0;
    load_keys_ = 0;
    uint64_t b_size = 1000;

    int64_t cur_key = 0;

    std::ifstream in(data_path);
    std::vector<std::string> values;

    while (load_data_size < target_data_size) {
      std::string cur_value;
      std::getline(in, cur_value);
      values.push_back(cur_value);
      load_data_size += cur_value.length();
    }

    LOG(stderr, "Loading...\n");

    auto start = high_resolution_clock::now();
    auto b_start = start;
#ifdef PROFILE
    Profiler::StartProfiling();
#endif
    for (auto& cur_value : values) {
      shard_->Append(cur_key++, cur_value);
      load_end_offset_ += cur_value.length();
      load_keys_++;

      // Periodically print out statistics
      if (load_keys_ % b_size == 0) {
        auto b_end = high_resolution_clock::now();
        auto elapsed_us = duration_cast<nanoseconds>(b_end - b_start).count();
        double avg_latency = (double) elapsed_us / (double) b_size;
        double completion = 100.0 * (double) (load_end_offset_)
            / (double) (target_data_size);
        LOG(stderr,
            "\033[A\033[2KLoading: %2.02lf%% (%9lld B). Avg latency: %.2lf ns\n",
            completion, load_end_offset_, avg_latency);
        b_start = high_resolution_clock::now();
      }
    }
#ifdef PROFILE
    Profiler::StopProfiling();
#endif

    // Print end of load statistics
    auto end = high_resolution_clock::now();
    auto elapsed_us = duration_cast<nanoseconds>(end - start).count();
    double avg_latency = (double) elapsed_us / (double) load_keys_;

    LOG(stderr,
        "\033[A\033[2KLoaded %lld key-value pairs (%lld B). Avg latency: %lf ns\n",
        load_keys_, load_end_offset_, avg_latency);

    if (dump) {
      LOG(stderr, "Dumping data structures to disk...");
      shard_->Dump(data_path + ".logstore");
      LOG(stderr, "Done.\n");
    }
  } else if (mode == 1) {
    LOG(stderr, "Loading...\n");
    shard_->Load(data_path + ".logstore");
    load_keys_ = shard_->GetNumKeys();
    load_end_offset_ = shard_->GetSize();
    LOG(stderr, "Loaded %lld key-value pairs.\n", load_keys_);
  } else {
    LOG(stderr, "Invalid mode: %d\n", mode);
    exit(-1);
  }

  if (shard_->GetSize() != load_end_offset_) {
    LOG(stderr, "Inconsistency: expected size = %lld, actual size %lld\n",
        load_end_offset_, shard_->GetSize());
  }

}

void MicroBenchmark::BenchmarkGetLatency() {
  // Generate queries
  LOG(stderr, "Generating queries...");
  std::vector<int64_t> keys;

  for (int64_t i = 0; i < kWarmupCount + kMeasureCount; i++) {
    int64_t key = rand() % load_keys_;
    keys.push_back(key);
  }

  LOG(stderr, "Done.\n");

  std::ofstream result_stream("latency_get");

  char result[10 * 1024];
  // Warmup
  LOG(stderr, "Warming up for %llu queries...\n", kWarmupCount);
  for (uint64_t i = 0; i < kWarmupCount; i++) {
    shard_->Get(result, keys[i]);
  }
  LOG(stderr, "Warmup complete.\n");

  // Measure
  LOG(stderr, "Measuring for %llu queries...\n", kMeasureCount);
  for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
    auto t0 = high_resolution_clock::now();
    shard_->Get(result, keys[i]);
    auto t1 = high_resolution_clock::now();
    auto tdiff = duration_cast<nanoseconds>(t1 - t0).count();
    result_stream << keys[i] << "\t" << tdiff << "\n";
  }
  LOG(stderr, "Measure complete.\n");
  result_stream.close();
}

void MicroBenchmark::BenchmarkSearchLatency() {
  LOG(stderr, "Reading queries...");
  std::vector<std::string> queries;

  std::ifstream in(data_path_ + ".queries");
  std::string query;
  while (queries.size() < kWarmupCount + kMeasureCount
      && std::getline(in, query)) {
    queries.push_back(query);
  }

  LOG(stderr, "Done.\n");

  std::ofstream result_stream("latency_search");

  // Warmup
  LOG(stderr, "Warming up for %llu queries...\n", kWarmupCount);
  for (uint64_t i = 0; i < kWarmupCount; i++) {
    std::string query = queries[i % queries.size()];
    std::set<int64_t> results;
    shard_->Search(results, query);
  }
  LOG(stderr, "Warmup complete.\n");

  // Measure
  LOG(stderr, "Measuring for %llu queries...\n", kMeasureCount);
  for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
    std::string query = queries[i % queries.size()];
    std::set<int64_t> results;
    auto t0 = high_resolution_clock::now();
    shard_->Search(results, query);
    auto t1 = high_resolution_clock::now();
    auto tdiff = duration_cast<nanoseconds>(t1 - t0).count();
    result_stream << results.size() << "\t" << tdiff << "\n";
  }
  LOG(stderr, "Measure complete.\n");
  result_stream.close();
}

void MicroBenchmark::BenchmarkAppendLatency() {
  // Generate queries
  LOG(stderr, "Generating queries...");
  std::vector<std::string> values;

  std::ifstream in(data_path_);
  in.seekg(load_end_offset_);
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
    int ret = shard_->Append(cur_key++, cur_value);
  }
  LOG(stderr, "Warmup complete.\n");

  // Measure
  LOG(stderr, "Measuring for %llu queries...\n", kMeasureCount);
  for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
    std::string cur_value = values[i];
    auto t0 = high_resolution_clock::now();
    int ret = shard_->Append(cur_key++, cur_value);
    auto t1 = high_resolution_clock::now();
    auto tdiff = duration_cast<nanoseconds>(t1 - t0).count();
    result_stream << (cur_key - 1) << "\t" << tdiff << "\n";
  }
  LOG(stderr, "Measure complete.\n");
  result_stream.close();
}

void MicroBenchmark::BenchmarkDeleteLatency() {
  // Generate queries
  LOG(stderr, "Generating queries...");
  std::vector<int64_t> keys;

  for (int64_t i = 0; i < kWarmupCount + kMeasureCount; i++) {
    int64_t key = rand() % load_keys_;
    keys.push_back(key);
  }

  LOG(stderr, "Done.\n");

  std::ofstream result_stream("latency_delete");

  // Warmup
  LOG(stderr, "Warming up for %llu queries...\n", kWarmupCount);
  for (uint64_t i = 0; i < kWarmupCount; i++) {
    shard_->Delete(keys[i]);
  }
  LOG(stderr, "Warmup complete.\n");

  // Measure
  LOG(stderr, "Measuring for %llu queries...\n", kMeasureCount);
  for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
    auto t0 = high_resolution_clock::now();
    shard_->Delete(keys[i]);
    auto t1 = high_resolution_clock::now();
    auto tdiff = duration_cast<nanoseconds>(t1 - t0).count();
    result_stream << keys[i] << "\t" << tdiff << "\n";
  }
  LOG(stderr, "Measure complete.\n");
  result_stream.close();
}

void MicroBenchmark::BenchmarkThroughput(const double get_f,
                                         const double search_f,
                                         const double append_f,
                                         const double delete_f,
                                         const uint32_t num_clients) {

  if (get_f + search_f + append_f + delete_f != 1.0) {
    LOG(stderr, "Query fractions must add up to 1.0. Sum = %lf\n",
        get_f + search_f + append_f + delete_f);
    return;
  }

  const double get_m = get_f, search_m = get_f + search_f, append_m = get_f
      + search_f, delete_m = 1.0;

  std::condition_variable cvar;
  std::vector<std::thread> threads;

  for (uint32_t i = 0; i < num_clients; i++) {
    threads.push_back(
        std::move(
            std::thread([&] {
              std::vector<int64_t> keys;
              std::vector<std::string> values, terms;

              std::ifstream in_s(data_path_ + ".queries");
              std::ifstream in_a(data_path_);
              int64_t cur_key = load_keys_;
              std::string term, value;
              std::vector<uint32_t> query_types;
              LOG(stderr, "Generating queries...\n");
              for (int64_t i = 0; i < kThreadQueryCount; i++) {
                int64_t key = RandomInteger(0, load_keys_);
                std::getline(in_s, term);
                std::getline(in_a, value);

                keys.push_back(key);
                terms.push_back(term);
                values.push_back(value);

                double r = RandomDouble(0, 1);
                if (r <= get_m) {
                  query_types.push_back(0);
                } else if (r <= search_m) {
                  query_types.push_back(1);
                } else if (r <= append_m) {
                  query_types.push_back(2);
                } else if (r <= delete_m) {
                  query_types.push_back(3);
                }
              }

              std::random_shuffle(keys.begin(), keys.end(), MicroBenchmark::RandomIndex);
              std::random_shuffle(terms.begin(), terms.end(), MicroBenchmark::RandomIndex);
              LOG(stderr, "Done.\n");

              double query_thput = 0;
              double key_thput = 0;
              char get_res[10*1024];

              try {
                // Warmup phase
                long i = 0;
                long num_keys = 0;
                TimeStamp warmup_start = GetTimestamp();
                while (GetTimestamp() - warmup_start < kWarmupTime) {
                  QUERY(i, num_keys);
                  i++;
                }

                // Measure phase
                i = 0;
                TimeStamp start = GetTimestamp();
                while (GetTimestamp() - start < kMeasureTime) {
                  QUERY(i, num_keys);
                  i++;
                }
                TimeStamp end = GetTimestamp();
                double totsecs = (double) (end - start) / (1000.0 * 1000.0);
                query_thput = ((double) i / totsecs);
                key_thput = ((double) num_keys / totsecs);

                // Cooldown phase
                i = 0;
                TimeStamp cooldown_start = GetTimestamp();
                while (GetTimestamp() - cooldown_start < kCooldownTime) {
                  QUERY(i, num_keys);
                  i++;
                }

              } catch (std::exception &e) {
                LOG(stderr, "Throughput thread ended prematurely.\n");
              }

              LOG(stderr, "Throughput: %lf\n", query_thput);

              std::ofstream ofs;
              char output_file[100];
              sprintf(output_file, "throughput_%.2f_%.2f_%.2f_%.2f_%d", get_f, search_f, append_f, delete_f, num_clients);
              ofs.open(output_file, std::ofstream::out | std::ofstream::app);
              ofs << query_thput << "\t" << key_thput << "\n";
              ofs.close();

            })));
  }

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
  if (argc < 2 || argc > 8) {
    PrintUsage(argv[0]);
    return -1;
  }

  int c;
  std::string bench_type = "latency-get";
  int mode = 0, num_clients = 1;
  bool dump = false;
  while ((c = getopt(argc, argv, "b:m:n:d")) != -1) {
    switch (c) {
      case 'b':
        bench_type = std::string(optarg);
        break;
      case 'm':
        mode = atoi(optarg);
        break;
      case 'n':
        num_clients = atoi(optarg);
        break;
      case 'd':
        dump = true;
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

  MicroBenchmark ls_bench(data_path, mode, dump);
  if (bench_type == "latency-get") {
    ls_bench.BenchmarkGetLatency();
  } else if (bench_type == "latency-search") {
    ls_bench.BenchmarkSearchLatency();
  } else if (bench_type == "latency-append") {
    ls_bench.BenchmarkAppendLatency();
  } else if (bench_type == "latency-delete") {
    ls_bench.BenchmarkDeleteLatency();
  } else if (bench_type.find("throughput") == 0) {
    std::vector<std::string> tokens = Split(bench_type, '-');
    if (tokens.size() != 5) {
      LOG(stderr, "Error: Incorrect throughput benchmark format.\n");
      return -1;
    }
    double get_f = atof(tokens[1].c_str());
    double search_f = atof(tokens[2].c_str());
    double append_f = atof(tokens[3].c_str());
    double delete_f = atof(tokens[4].c_str());
    LOG(stderr,
        "get_f = %.2lf, search_f = %.2lf, append_f = %.2lf, delete_f = %.2lf, num_clients = %d\n",
        get_f, search_f, append_f, delete_f, num_clients);
    ls_bench.BenchmarkThroughput(get_f, search_f, append_f, delete_f,
                                 num_clients);
  } else {
    LOG(stderr, "Unknown benchmark type: %s\n", bench_type.c_str());
  }

  return 0;
}

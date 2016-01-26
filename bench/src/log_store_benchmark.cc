#include "log_store_benchmark.h"

#include <chrono>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <thread>
#include <condition_variable>
#include <unistd.h>
#include <sstream>

#define QUERY(i) {\
  std::string get_res;\
  std::set<int64_t> search_res;\
  if (query_types[i % kThreadQueryCount] == 0) {\
    client->Get(get_res, keys[i % keys.size()]);\
  } else if (query_types[i % kThreadQueryCount] == 1) {\
    client->Search(search_res, terms[i % terms.size()]);\
  } else {\
    if (client->Append(cur_key, values[i % values.size()]) != 0) {\
      fprintf(stderr, "Log is full\n");\
      break;\
    }\
  }\
}

LogStoreBenchmark::LogStoreBenchmark(std::string& data_path, int mode) {

  char resolved_path[100];
  realpath(data_path.c_str(), resolved_path);
  data_path_ = resolved_path;

  BenchmarkConnection cx("localhost", 11002);

  if (mode == 0) {
    /** Load the data
     *  =============
     *
     *  We load the log store with ~250MB of data (~25% of total capacity).
     */

    const int64_t target_data_size = 250 * 1024 * 1024;
    load_end_offset_ = 0;
    load_keys_ = 0;

    int64_t cur_key = 0;

    std::ifstream in(data_path);

    fprintf(stderr, "Loading...\n");

    TimeStamp start = GetTimestamp();
    TimeStamp batch_start = start;
    while (load_end_offset_ < target_data_size) {
      std::string cur_value;
      std::getline(in, cur_value);
      cx.client->Append(cur_key++, cur_value);
      load_end_offset_ += cur_value.length();
      load_keys_++;

      // Periodically print out statistics
      if (load_keys_ % 1000 == 0) {
        TimeStamp batch_end = GetTimestamp();
        auto elapsed_us = batch_end - batch_start;
        double avg_latency = (double) elapsed_us / 1000.0;
        double completion = 100.0 * (double) (load_end_offset_)
            / (double) (target_data_size);
        fprintf(stderr,
                "\033[A\033[2KLoading: %2.02lf%%. Avg latency: %.2lf us\n",
                completion, avg_latency);
        batch_start = GetTimestamp();
      }
    }

    // Print end of load statistics
    TimeStamp end = GetTimestamp();
    auto elapsed_us = end - start;
    double avg_latency = (double) elapsed_us / (double) load_keys_;

    fprintf(
        stderr,
        "\033[A\033[2KLoaded %ld key-value pairs (%lld B). Avg latency: %lf us\n",
        load_keys_, load_end_offset_, avg_latency);

    fprintf(stderr, "Dumping data structures to disk...");
    cx.client->Dump(data_path + ".logstore");
    fprintf(stderr, "Done.\n");
  } else if (mode == 1) {
    fprintf(stderr, "Loading...\n");
    cx.client->Load(data_path + ".logstore");
    load_keys_ = cx.client->GetNumKeys();
    load_end_offset_ = cx.client->GetSize();
    fprintf(stderr, "Loaded %ld key-value pairs.\n", load_keys_);
  } else {
    fprintf(stderr, "Invalid mode: %d\n", mode);
    exit(-1);
  }

  if (cx.client->GetSize() != load_end_offset_) {
    fprintf(stderr, "Inconsistency: expected size = %lld, actual size %lld\n",
            load_end_offset_, cx.client->GetSize());
  }

}

void LogStoreBenchmark::BenchmarkGetLatency() {
  // Generate queries
  fprintf(stderr, "Generating queries...");
  std::vector<int64_t> keys;

  for (int64_t i = 0; i < kWarmupCount + kMeasureCount; i++) {
    int64_t key = rand() % load_keys_;
    keys.push_back(key);
  }

  fprintf(stderr, "Done.\n");

  TimeStamp t0, t1, tdiff;
  std::ofstream result_stream("latency_get");
  BenchmarkConnection cx("localhost", 11002);
  auto client = cx.client;

  // Warmup
  fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
  for (uint64_t i = 0; i < kWarmupCount; i++) {
    std::string result;
    client->Get(result, keys[i]);
  }
  fprintf(stderr, "Warmup complete.\n");

  // Measure
  fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
  for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
    std::string result;
    t0 = GetTimestamp();
    client->Get(result, keys[i]);
    t1 = GetTimestamp();
    tdiff = t1 - t0;
    result_stream << keys[i] << "\t" << tdiff << "\n";
  }
  fprintf(stderr, "Measure complete.\n");
  result_stream.close();
}

void LogStoreBenchmark::BenchmarkSearchLatency() {
  fprintf(stderr, "Reading queries...");
  std::vector<std::string> queries;

  std::ifstream in(data_path_ + ".queries");
  std::string query;
  while (queries.size() < kWarmupCount + kMeasureCount
      && std::getline(in, query)) {
    queries.push_back(query);
  }

  fprintf(stderr, "Done.\n");

  TimeStamp t0, t1, tdiff;
  std::ofstream result_stream("latency_search");
  BenchmarkConnection cx("localhost", 11002);
  auto client = cx.client;

  // Warmup
  fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
  for (uint64_t i = 0; i < kWarmupCount; i++) {
    std::string query = queries[i % queries.size()];
    std::set<int64_t> results;
    client->Search(results, query);
  }
  fprintf(stderr, "Warmup complete.\n");

  // Measure
  fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
  for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
    std::string query = queries[i % queries.size()];
    std::set<int64_t> results;
    t0 = GetTimestamp();
    client->Search(results, query);
    t1 = GetTimestamp();
    tdiff = t1 - t0;
    result_stream << results.size() << "\t" << tdiff << "\n";
  }
  fprintf(stderr, "Measure complete.\n");
  result_stream.close();
}

void LogStoreBenchmark::BenchmarkAppendLatency() {
  // Generate queries
  fprintf(stderr, "Generating queries...");
  std::vector<std::string> values;

  std::ifstream in(data_path_);
  in.seekg(load_end_offset_);
  for (int64_t i = 0; i < kWarmupCount + kMeasureCount; i++) {
    std::string cur_value;
    std::getline(in, cur_value);
    values.push_back(cur_value);
  }
  int64_t cur_key = load_keys_;

  fprintf(stderr, "Done.\n");

  TimeStamp t0, t1, tdiff;
  std::ofstream result_stream("latency_append");
  BenchmarkConnection cx("localhost", 11002);
  auto client = cx.client;

  // Warmup
  fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
  for (uint64_t i = 0; i < kWarmupCount; i++) {
    std::string cur_value = values[i];
    int ret = client->Append(cur_key++, cur_value);
    if (ret != 0) {
      fprintf(stderr, "Log store is full.\n");
      break;
    }
  }
  fprintf(stderr, "Warmup complete.\n");

  // Measure
  fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
  for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
    std::string cur_value = values[i];
    t0 = GetTimestamp();
    int ret = client->Append(cur_key++, cur_value);
    t1 = GetTimestamp();
    tdiff = t1 - t0;
    if (ret != 0) {
      fprintf(stderr, "Log store is full.\n");
      break;
    }
    result_stream << (cur_key - 1) << "\t" << tdiff << "\n";
  }
  fprintf(stderr, "Measure complete.\n");
  result_stream.close();
}

void LogStoreBenchmark::BenchmarkThroughput(double get_f, double search_f,
                                            double append_f,
                                            uint32_t num_clients) {

  if (get_f + search_f + append_f != 1.0) {
    fprintf(stderr, "Query fractions must add up to 1.0. Sum = %lf\n",
            get_f + search_f + append_f);
    return;
  }

  const double get_m = get_f, search_m = get_f + search_f, append_m = 1.0;

  std::mutex mtx;
  std::condition_variable cvar;
  std::vector<std::thread> threads;
  uint32_t num_ready = 0;

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
              fprintf(stderr, "Generating queries...\n");
              for (int64_t i = 0; i < kThreadQueryCount; i++) {
                int64_t key = rand() % load_keys_;
                std::getline(in_s, term);
                std::getline(in_a, value);

                keys.push_back(key);
                terms.push_back(term);
                values.push_back(value);

                double r = ((double) rand() / (RAND_MAX));
                if (r <= get_m) {
                  query_types.push_back(0);
                } else if (r <= search_m) {
                  query_types.push_back(1);
                } else if (r <= append_m) {
                  query_types.push_back(2);
                }
              }
              fprintf(stderr, "Done.\n");

              std::unique_lock<std::mutex> lck(mtx);
              num_ready++;
              while (num_ready < num_clients) {
                cvar.wait(lck);
              }

              double thput = 0;
              BenchmarkConnection cx("localhost", 11002);
              auto client = cx.client;

              try {
                // Warmup phase
                long i = 0;
                TimeStamp warmup_start = GetTimestamp();
                while (GetTimestamp() - warmup_start < kWarmupTime) {
                  QUERY(i);
                  i++;
                }

                // Measure phase
                i = 0;
                TimeStamp start = GetTimestamp();
                while (GetTimestamp() - start < kMeasureTime) {
                  QUERY(i);
                  i++;
                }
                TimeStamp end = GetTimestamp();
                double totsecs = (double) (end - start) / (1000.0 * 1000.0);
                thput = ((double) i / totsecs);

                // Cooldown phase
                i = 0;
                TimeStamp cooldown_start = GetTimestamp();
                while (GetTimestamp() - cooldown_start < kCooldownTime) {
                  QUERY(i);
                  i++;
                }

              } catch (std::exception &e) {
                fprintf(stderr, "Throughput thread ended prematurely.\n");
              }

              fprintf(stderr, "Throughput: %lf\n", thput);

              std::ofstream ofs;
              char output_file[100];
              sprintf(output_file, "throughput_%.2f_%.2f_%.2f", get_f, search_f, append_f);
              ofs.open(output_file, std::ofstream::out | std::ofstream::app);
              ofs << thput << "\n";
              ofs.close();

            })));
  }

  for (auto& th : threads) {
    th.join();
  }

}

void PrintUsage(char *exec) {
  fprintf(stderr, "Usage: %s [-b bench-type] [-m mode] data-path\n", exec);
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
  int mode = 0;
  while ((c = getopt(argc, argv, "b:m:")) != -1) {
    switch (c) {
      case 'b':
        bench_type = std::string(optarg);
        break;
      case 'm':
        mode = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Could not parse command line arguments.\n");
    }
  }

  if (optind == argc) {
    PrintUsage(argv[0]);
    return -1;
  }

  std::string data_path = std::string(argv[optind]);

  LogStoreBenchmark ls_bench(data_path, mode);
  if (bench_type == "latency-get") {
    ls_bench.BenchmarkGetLatency();
  } else if (bench_type == "latency-search") {
    ls_bench.BenchmarkSearchLatency();
  } else if (bench_type == "latency-append") {
    ls_bench.BenchmarkAppendLatency();
  } else if (bench_type.find("throughput") == 0) {
    std::vector<std::string> tokens = Split(bench_type, '-');
    if (tokens.size() != 4) {
      fprintf(stderr, "Error: Incorrect throughput benchmark format.\n");
    }
    double get_f = atof(tokens[1].c_str());
    double search_f = atof(tokens[2].c_str());
    double append_f = atof(tokens[3].c_str());
    fprintf(stderr, "get_f = %.2lf, search_f = %.2lf, append_f = %.2lf\n",
            get_f, search_f, append_f);
    ls_bench.BenchmarkThroughput(get_f, search_f, append_f);
  } else {
    fprintf(stderr, "Unknown benchmark type: %s\n", bench_type.c_str());
  }

  return 0;
}

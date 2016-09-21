#ifndef MICRO_BENCHMARK_H_
#define MICRO_BENCHMARK_H_

#include <chrono>
#include <ctime>
#include <sys/time.h>
#include <random>
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

#include "cpu_utilization.h"
#include "logstore.h"

#ifdef NO_LOG
#define LOG(out, fmt, ...)
#else
#define LOG(out, fmt, ...) fprintf(out, fmt, ##__VA_ARGS__)
#endif

using namespace ::slog;
using namespace ::std::chrono;

class rate_limiter {
 public:
  rate_limiter(double ops_per_sec) {
    double tmp = (((double) 1e6) / ops_per_sec);
    min_us_per_op = tmp;
    local_ops_ = 0;
    last_ts_ = high_resolution_clock::now();
    LOG(stderr, "1op per %lld ns.\n", min_us_per_op);
  }

  uint64_t limit() {
    local_ops_++;

    high_resolution_clock::time_point now = high_resolution_clock::now();
    auto us_since_last_op = duration_cast<microseconds>(now - last_ts_).count();
    if (us_since_last_op < min_us_per_op) {
      usleep(min_us_per_op - us_since_last_op);
    }
    last_ts_ = high_resolution_clock::now();

    return local_ops_;
  }

  uint64_t local_ops() {
    return local_ops_;
  }

 private:
  high_resolution_clock::time_point last_ts_;
  uint64_t local_ops_;
  uint64_t min_us_per_op;
};

class rate_limiter_inf {
 public:
  rate_limiter_inf(double ops_per_sec) {
    local_ops_ = 0;
  }

  uint64_t limit() {
    return ++local_ops_;
  }

  uint64_t local_ops() {
    return local_ops_;
  }

 private:
  uint64_t local_ops_;
};

template<class rlimiter = rate_limiter_inf>
class filter_benchmark {
 public:
  typedef uint64_t (filter_benchmark::*query_func)();

  typedef unsigned long long int timestamp_t;

  static const uint64_t kWarmupCount = 1000;
  static const uint64_t kMeasureCount = 100000;
  static const uint64_t kCooldownCount = 1000;

  static const uint64_t kWarmupTime = 5000000;
  static const uint64_t kMeasureTime = 10000000;
  static const uint64_t kCooldownTime = 5000000;

  static const uint64_t kThreadQueryCount = 75000;

  filter_benchmark(std::string& data_path, std::string& attr_path) {
    char resolved_path[100];
    realpath(data_path.c_str(), resolved_path);
    data_path_ = std::string(resolved_path);
    realpath(attr_path.c_str(), resolved_path);
    attr_path_ = std::string(resolved_path);

    logstore_ = new log_store();

    LOG(stderr, "Loading data...\n");
    load_data();

    LOG(stderr, "Initialization complete.\n");
  }

  uint64_t insert_packet(log_store::handle* handle, uint64_t idx) {
    tokens tkns;
    init_tokens(tkns, idx);
    return handle->insert(datas_[idx], datalens_[idx], tkns) + 1;
  }

  void init_tokens(tokens& tkns, uint64_t idx) {
    tkns.time = (unsigned char*) (&timestamps_[idx]);
    tkns.src_ip = (unsigned char*) (&srcips_[idx]);
    tkns.dst_ip = (unsigned char*) (&dstips_[idx]);
    tkns.src_prt = (unsigned char*) (&sports_[idx]);
    tkns.dst_prt = (unsigned char*) (&dports_[idx]);
  }

  uint32_t parse_ip(std::string& ip) {
    uint32_t byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0;
    sscanf(ip.c_str(), "%u.%u.%u.%u", &byte3, &byte2, &byte1, &byte0);
    return byte3 | byte2 << 8 | byte1 << 16 | byte0 << 24;
  }

  uint32_t parse_time(uint32_t time) {
    unsigned char* timearr = (unsigned char*) (&time);
    return timearr[2] | timearr[1] << 8 | timearr[0] << 16;
  }

  uint16_t parse_port(uint16_t port) {
    unsigned char* portarr = (unsigned char*) (&port);
    return portarr[1] | portarr[0] << 8;
  }

  void load_data() {
    std::ifstream ind(data_path_);
    std::ifstream ina(attr_path_);
    std::string attr_line;
    log_store::handle* handle = logstore_->get_handle();
    LOG(stderr, "Reading from path data=%s, attr=%s\n", data_path_.c_str(),
        attr_path_.c_str());

    while (std::getline(ina, attr_line)) {
      uint32_t ts;
      std::string srcip, dstip;
      uint16_t sport, dport;
      uint16_t len;
      std::stringstream attr_stream(attr_line);
      attr_stream >> ts >> len >> srcip >> dstip >> sport >> dport;
      unsigned char* data = new unsigned char[len];
      ind.read((char*) data, len);
      timestamps_.push_back(parse_time(ts));
      srcips_.push_back(parse_ip(srcip));
      dstips_.push_back(parse_ip(srcip));
      sports_.push_back(parse_port(sport));
      dports_.push_back(parse_port(dport));
      datas_.push_back(data);
      datalens_.push_back(len);
      etime_ = ts;
    }

    last_ = timestamps_.size() - 1;
    logstore_->set_params((unsigned char*) &srcips_[last_],
                          (unsigned char*) &dstips_[last_],
                          (unsigned char*) &sports_[last_],
                          (unsigned char*) &dports_[last_]);

    for (size_t i = 0; i <= last_; i++) {
      insert_packet(handle, i);
    }

    LOG(stderr, "Loaded %zu packets.\n", datas_.size());
  }

  uint64_t q1() {
    return logstore_->q1((unsigned char*) &srcips_[last_],
                         (unsigned char*) &dstips_[last_]);
  }

  uint64_t q2() {
    std::set<uint32_t> sips;
    logstore_->q2(sips, (unsigned char*) &dstips_[last_]);
    return sips.size();
  }

  uint64_t q3() {
    std::set<uint32_t> rids;
    logstore_->q3(rids);
    return rids.size();
  }

  uint64_t q4() {
    std::set<uint32_t> rids;
    logstore_->q4(rids, (unsigned char*) &srcips_[last_],
                  (unsigned char*) &dports_[last_]);
    return rids.size();
  }

  uint64_t q5() {
    std::set<uint32_t> rids;
    logstore_->q5(rids, (unsigned char*) &srcips_[last_],
                  (unsigned char*) &dstips_[last_],
                  (unsigned char*) &sports_[last_],
                  (unsigned char*) &dports_[last_]);
    return rids.size();
  }

  uint64_t q6() {
    std::set<uint32_t> rids;
    logstore_->q6(rids, (unsigned char*) &srcips_[last_]);
    return rids.size();
  }

  uint64_t q1_fast() {
    return logstore_->q1_fast();
  }

  uint64_t q2_fast() {
    std::set<uint32_t> results;
    logstore_->q2_fast(results);
    return results.size();
  }

  uint64_t q3_fast() {
    std::set<uint32_t> results;
    logstore_->q3_fast(results);
    return results.size();
  }

  uint64_t q4_fast() {
    std::set<uint32_t> results;
    logstore_->q4_fast(results);
    return results.size();
  }

  uint64_t q5_fast() {
    std::set<uint32_t> results;
    logstore_->q5_fast(results);
    return results.size();
  }

  uint64_t q6_fast() {
    std::set<uint32_t> results;
    logstore_->q6_fast(results);
    return results.size();
  }

  void latency_q1() {
    // 1000 queries
    std::ofstream q1_out("latency_q1");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      auto start = high_resolution_clock::now();
      uint64_t count = q1();
      auto end = high_resolution_clock::now();
      q1_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q1_out.close();
  }

  void latency_q2() {
    std::ofstream q2_out("latency_q2");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      auto start = high_resolution_clock::now();
      uint64_t count = q2();
      auto end = high_resolution_clock::now();
      q2_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q2_out.close();
  }

  void latency_q3() {
    std::ofstream q3_out("latency_q3");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      auto start = high_resolution_clock::now();
      uint64_t count = q3();
      auto end = high_resolution_clock::now();
      q3_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q3_out.close();
  }

  void latency_q4() {
    std::ofstream q4_out("latency_q4");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      auto start = high_resolution_clock::now();
      uint64_t count = q4();
      auto end = high_resolution_clock::now();
      q4_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q4_out.close();
  }

  void latency_q5() {
    std::ofstream q5_out("latency_q5");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      auto start = high_resolution_clock::now();
      uint64_t count = q4();
      auto end = high_resolution_clock::now();
      q5_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q5_out.close();
  }

  void latency_q6() {
    std::ofstream q6_out("latency_q6");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      auto start = high_resolution_clock::now();
      uint64_t count = q1();
      auto end = high_resolution_clock::now();
      q6_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q6_out.close();
  }

  void latency_q1_fast() {
    // 1000 queries
    std::ofstream q1_out("latency_q1_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      auto start = high_resolution_clock::now();
      uint64_t count = q1_fast();
      auto end = high_resolution_clock::now();
      q1_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q1_out.close();
  }

  void latency_q2_fast() {
    std::ofstream q2_out("latency_q2_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      auto start = high_resolution_clock::now();
      uint64_t count = q2_fast();
      auto end = high_resolution_clock::now();
      q2_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q2_out.close();
  }

  void latency_q3_fast() {
    std::ofstream q3_out("latency_q3_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      auto start = high_resolution_clock::now();
      uint64_t count = q3_fast();
      auto end = high_resolution_clock::now();
      q3_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q3_out.close();
  }

  void latency_q4_fast() {
    std::ofstream q4_out("latency_q4_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      auto start = high_resolution_clock::now();
      uint64_t count = q4_fast();
      auto end = high_resolution_clock::now();
      q4_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q4_out.close();
  }

  void latency_q5_fast() {
    std::ofstream q5_out("latency_q5_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      auto start = high_resolution_clock::now();
      uint64_t count = q5_fast();
      auto end = high_resolution_clock::now();
      q5_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q5_out.close();
  }

  void latency_q6_fast() {
    std::ofstream q6_out("latency_q6_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      auto start = high_resolution_clock::now();
      uint64_t count = q6_fast();
      auto end = high_resolution_clock::now();
      q6_out << count << "\t" << duration_cast<nanoseconds>(end - start).count()
             << "\n";
    }
    q6_out.close();
  }

  // Latency benchmarks
  void latency_all() {
    latency_q1();
    latency_q2();
    latency_q3();
    latency_q4();
    latency_q5();
    latency_q6();
    latency_q1_fast();
    latency_q2_fast();
    latency_q3_fast();
    latency_q4_fast();
    latency_q5_fast();
    latency_q6_fast();
  }

  void query_throughput(query_func f, const uint32_t num_threads,
                        const uint64_t timebound, const uint64_t rate_limit,
                        const std::string& tag) {

    std::vector<std::thread> threads;
    double local_rate_limit = (double) rate_limit / (double) num_threads;

    LOG(stderr, "Setting timebound to %llu us\n", timebound);
    for (uint32_t i = 0; i < num_threads; i++) {
      threads.push_back(
          std::move(
              std::thread(
                  [i, timebound, local_rate_limit, tag, &f, this] {
                    LOG(stderr, "Starting benchmark.\n");
                    rlimiter* limiter = new rlimiter(local_rate_limit);
                    double throughput = 0;

                    try {
                      uint64_t total_ops = 0;
                      timestamp_t start = get_timestamp();
                      while (get_timestamp() - start < timebound) {
                        (this->*f)();
                        total_ops = limiter->limit();
                      }
                      timestamp_t end = get_timestamp();
                      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
                      throughput = ((double) limiter->local_ops() / totsecs);
                      LOG(stderr, "Thread #%u finished in %lf s. Throughput: %lf.\n", i, totsecs, throughput);
                    } catch (std::exception &e) {
                      LOG(stderr, "Throughput thread ended prematurely.\n");
                    }

                    std::ofstream ofs("write_throughput_" + std::to_string(i) + "_" + tag);
                    ofs << throughput << "\n";
                    ofs.close();
                    delete limiter;
                  })));
    }

    std::thread cpu_measure_thread([&] {
      timestamp_t start = get_timestamp();
      std::ofstream util_stream("cpu_utilization" + tag);
      cpu_utilization util;
      while (get_timestamp() - start < timebound) {
        sleep(1);
        util_stream << util.current() << "\n";
      }
      util_stream.close();
    });

    for (auto& th : threads) {
      th.join();
    }
    cpu_measure_thread.join();
  }

  void throughput_all(const uint32_t num_threads, const uint64_t timebound,
                      const uint64_t rate_limit) {

    query_throughput(&filter_benchmark::q1, num_threads, timebound, rate_limit,
                     "q1_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q2, num_threads, timebound, rate_limit,
                     "q2_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q3, num_threads, timebound, rate_limit,
                     "q3_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q4, num_threads, timebound, rate_limit,
                     "q4_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q5, num_threads, timebound, rate_limit,
                     "q5_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q6, num_threads, timebound, rate_limit,
                     "q6_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q1_fast, num_threads, timebound,
                     rate_limit, "q1_fast_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q2_fast, num_threads, timebound,
                     rate_limit, "q2_fast_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q3_fast, num_threads, timebound,
                     rate_limit, "q3_fast_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q4_fast, num_threads, timebound,
                     rate_limit, "q4_fast_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q5_fast, num_threads, timebound,
                     rate_limit, "q5_fast_" + std::to_string(rate_limit));
    query_throughput(&filter_benchmark::q6_fast, num_threads, timebound,
                     rate_limit, "q6_fast_" + std::to_string(rate_limit));

  }

 private:
  static timestamp_t get_timestamp() {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_usec + (timestamp_t) now.tv_sec * 1000000;
  }

  std::string data_path_;
  std::string attr_path_;

  std::vector<uint32_t> timestamps_;
  std::vector<uint32_t> srcips_;
  std::vector<uint32_t> dstips_;
  std::vector<uint16_t> sports_;
  std::vector<uint16_t> dports_;
  std::vector<unsigned char*> datas_;
  std::vector<uint16_t> datalens_;
  uint32_t etime_;
  size_t last_;

  log_store *logstore_;
};

#endif

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

#include "logstore.h"

#ifdef NO_LOG
#define LOG(out, fmt, ...)
#else
#define LOG(out, fmt, ...) fprintf(out, fmt, ##__VA_ARGS__)
#endif

using namespace ::slog;
using namespace ::std::chrono;

class filter_benchmark {
 public:
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

    size_t last = timestamps_.size() - 1;
    logstore_->set_params((unsigned char*) &srcips_[last],
                          (unsigned char*) &dstips_[last],
                          (unsigned char*) &sports_[last],
                          (unsigned char*) &dports_[last]);

    for (size_t i = 0; i <= last; i++) {
      insert_packet(handle, i);
    }

    LOG(stderr, "Loaded %zu packets.\n", datas_.size());
  }

  void latency_q1() {
    // 1000 queries
    std::ofstream q1_out("latency_q1");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      timestamp_t start = get_timestamp();
      uint64_t count = logstore_->q1((unsigned char*) &srcips_[i],
                                     (unsigned char*) &dstips_[i]);
      timestamp_t end = get_timestamp();
      q1_out << count << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", count, (end - start));
    }
    q1_out.close();
  }

  void latency_q2() {
    std::ofstream q2_out("latency_q2");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      std::set<uint32_t> sips;
      timestamp_t start = get_timestamp();
      logstore_->q2(sips, (unsigned char*) &dstips_[i]);
      timestamp_t end = get_timestamp();
      q2_out << sips.size() << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", sips.size(),
              (end - start));
    }
    q2_out.close();
  }

  void latency_q3() {
    std::ofstream q3_out("latency_q3");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      std::set<uint32_t> rids;
      timestamp_t start = get_timestamp();
      logstore_->q3(rids, (unsigned char*) &dports_[i]);
      timestamp_t end = get_timestamp();
      q3_out << rids.size() << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", rids.size(),
              (end - start));
    }
    q3_out.close();
  }

  void latency_q4() {
    std::ofstream q4_out("latency_q4");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      std::set<uint32_t> rids;
      timestamp_t start = get_timestamp();
      logstore_->q4(rids, (unsigned char*) &srcips_[i],
                    (unsigned char*) &dports_[i]);
      timestamp_t end = get_timestamp();
      q4_out << rids.size() << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", rids.size(),
              (end - start));
    }
    q4_out.close();
  }

  void latency_q5() {
    std::ofstream q5_out("latency_q5");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      std::set<uint32_t> rids;
      timestamp_t start = get_timestamp();
      logstore_->q5(rids, (unsigned char*) &srcips_[i],
                    (unsigned char*) &dstips_[i], (unsigned char*) &sports_[i],
                    (unsigned char*) &dports_[i]);
      timestamp_t end = get_timestamp();
      q5_out << rids.size() << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", rids.size(),
              (end - start));
    }
    q5_out.close();
  }

  void latency_q6() {
    std::ofstream q6_out("latency_q6");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      std::set<uint32_t> rids;
      timestamp_t start = get_timestamp();
      logstore_->q6(rids, (unsigned char*) &srcips_[i]);
      timestamp_t end = get_timestamp();
      q6_out << rids.size() << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", rids.size(),
              (end - start));
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
  }

  void latency_q1_fast() {
    // 1000 queries
    std::ofstream q1_out("latency_q1_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      timestamp_t start = get_timestamp();
      uint64_t count = logstore_->q1_fast();
      timestamp_t end = get_timestamp();
      q1_out << count << "\t" << (end - start) << "\n";
    }
    q1_out.close();
  }

  void latency_q2_fast() {
    std::ofstream q2_out("latency_q2_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      std::set<uint32_t> sips;
      timestamp_t start = get_timestamp();
      logstore_->q2_fast(sips);
      timestamp_t end = get_timestamp();
      q2_out << sips.size() << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", sips.size(),
              (end - start));
    }
    q2_out.close();
  }

  void latency_q3_fast() {
    std::ofstream q3_out("latency_q3_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      std::set<uint32_t> rids;
      timestamp_t start = get_timestamp();
      logstore_->q3_fast(rids);
      timestamp_t end = get_timestamp();
      q3_out << rids.size() << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", rids.size(),
              (end - start));
    }
    q3_out.close();
  }

  void latency_q4_fast() {
    std::ofstream q4_out("latency_q4_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      std::set<uint32_t> rids;
      timestamp_t start = get_timestamp();
      logstore_->q4_fast(rids);
      timestamp_t end = get_timestamp();
      q4_out << rids.size() << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", rids.size(),
              (end - start));
    }
    q4_out.close();
  }

  void latency_q5_fast() {
    std::ofstream q5_out("latency_q5_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      std::set<uint32_t> rids;
      timestamp_t start = get_timestamp();
      logstore_->q5_fast(rids);
      timestamp_t end = get_timestamp();
      q5_out << rids.size() << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", rids.size(),
              (end - start));
    }
    q5_out.close();
  }

  void latency_q6_fast() {
    std::ofstream q6_out("latency_q6_fast");
    for (uint32_t i = timestamps_.size() - 1000; i < timestamps_.size(); i++) {
      uint64_t idx = rand() % timestamps_.size();
      std::set<uint32_t> rids;
      timestamp_t start = get_timestamp();
      logstore_->q6_fast(rids);
      timestamp_t end = get_timestamp();
      q6_out << rids.size() << "\t" << (end - start) << "\n";
      fprintf(stderr, "Count = %llu, Latency = %llu\n", rids.size(),
              (end - start));
    }
    q6_out.close();
  }

  void latency_fast() {
    latency_q1_fast();
    latency_q2_fast();
    latency_q3_fast();
    latency_q4_fast();
    latency_q5_fast();
    latency_q6_fast();
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

  log_store *logstore_;
};

#endif

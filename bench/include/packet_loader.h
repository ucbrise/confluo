#ifndef PACKETLOADER_H_
#define PACKETLOADER_H_

#include <chrono>
#include <sys/time.h>
#include <random>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

#include "logstore.h"

using namespace ::slog;
using namespace ::std::chrono;

class packet_loader {
 public:
  typedef unsigned long long int timestamp_t;
  static const uint64_t kReportRecordInterval = 1000000;

  uint64_t insert_packet(log_store::handle* handle, uint64_t idx) {
    tokens tkns;
    tkns.time = (unsigned char*) (&timestamps_[idx]);
    tkns.src_ip = (unsigned char*) (&srcips_[idx]);
    tkns.dst_ip = (unsigned char*) (&dstips_[idx]);
    tkns.src_prt = (unsigned char*) (&sports_[idx]);
    tkns.dst_prt = (unsigned char*) (&dports_[idx]);

    return handle->insert(datas_[idx], datalens_[idx], tkns) + 1;
  }

  uint32_t parse_ip(std::string& ip) {
    uint32_t byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0;
    sscanf(ip.c_str(), "%u.%u.%u.%u", &byte3, &byte2, &byte1, &byte0);
    return byte3 | byte2 << 8 | byte1 << 16 | byte0 << 24;
  }

  void print_time(uint32_t orig, uint32_t parsed) {
    unsigned char* orig_arr = (unsigned char*) (&orig);
    unsigned char* parsed_arr = (unsigned char*) (&parsed);

    fprintf(stderr, "Orig: %u,%u,%u,%u, Parsed: %u,%u,%u,%u\n", orig_arr[0],
            orig_arr[1], orig_arr[2], orig_arr[3], parsed_arr[0], parsed_arr[1],
            parsed_arr[2], parsed_arr[3]);
  }

  uint32_t parse_time(uint32_t time) {
    unsigned char* timearr = (unsigned char*) (&time);
    uint32_t ret = timearr[2] | timearr[1] << 8 | timearr[0] << 16;
    print_time(time, ret);
    return ret;
  }

  packet_loader(std::string& data_path, std::string& attr_path,
                std::string& hostname);

  void load_data();

  // Throughput benchmarks
  void load_packets(const uint32_t num_clients, const uint64_t timebound);

 private:
  static timestamp_t get_timestamp() {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_usec + (timestamp_t) now.tv_sec * 1000000;
  }

  std::string data_path_;
  std::string attr_path_;
  std::string hostname_;

  std::vector<uint32_t> timestamps_;
  std::vector<uint32_t> srcips_;
  std::vector<uint32_t> dstips_;
  std::vector<uint16_t> sports_;
  std::vector<uint16_t> dports_;
  std::vector<unsigned char*> datas_;
  std::vector<uint16_t> datalens_;

  log_store *logstore_;
};

#endif /* RAMCLOUDBENCHMARK_H_ */

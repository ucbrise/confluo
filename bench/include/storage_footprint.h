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
#include <unistd.h>

#include "cpu_utilization.h"
#include "logstore.h"

#ifdef NO_LOG
#define LOG(out, fmt, ...)
#else
#define LOG(out, fmt, ...) fprintf(out, fmt, ##__VA_ARGS__)
#endif

using namespace ::slog;

#define HDR_SIZE 54

class storage_footprint {
 public:
  storage_footprint(uint64_t packet_rate, uint64_t num_packets) {
    packet_rate_ = packet_rate;
    num_packets_ = num_packets;
    logstore_ = new log_store();
    LOG(stderr, "Initialization complete.\n");
  }

  uint64_t insert_packet(log_store::handle* handle, uint64_t completed_ops) {
    uint32_t time = gen_time(completed_ops);
    uint32_t src_ip = gen_ip();
    uint32_t dst_ip = gen_ip();
    uint16_t src_prt = gen_port();
    uint16_t dst_prt = gen_port();

    tokens tkns;
    tkns.time = (unsigned char*) (&time);
    tkns.src_ip = (unsigned char*) (&src_ip);
    tkns.dst_ip = (unsigned char*) (&dst_ip);
    tkns.src_prt = (unsigned char*) (&src_prt);
    tkns.dst_prt = (unsigned char*) (&dst_prt);

    return handle->insert(data, HDR_SIZE, tkns) + 1;
  }

  uint32_t gen_ip() {
    uint32_t byte3 = 10, byte2 = 0, byte1 = rand() % 256, byte0 = rand() % 256;
    return byte3 | byte2 << 8 | byte1 << 16 | byte0 << 24;
  }

  uint32_t gen_time(uint64_t completed_ops) {
    uint32_t cur_time = completed_ops / packet_rate_;
    unsigned char* timearr = (unsigned char*) (&cur_time);
    return timearr[2] | timearr[1] << 8 | timearr[0] << 16;
  }

  uint16_t gen_port() {
    uint16_t port = rand() % 8;
    unsigned char* portarr = (unsigned char*) (&port);
    return portarr[1] | portarr[0] << 8;
  }

  void load_packets() {
    tokens tkns;
    log_store::handle* handle = logstore_->get_handle();
    uint64_t report_marker = num_packets_ / 10;
    std::ofstream rfs("storage_footprint");
    LOG(stderr, "Starting load...\n");

    try {
      uint64_t completed_ops = 0;
      while (completed_ops < num_packets_) {
        completed_ops = insert_packet(handle, completed_ops);
        if (completed_ops % report_marker == 0) {
          rfs << completed_ops << "\t" << completed_ops << "\n";
        }
      }
    } catch (std::exception &e) {
      LOG(stderr, "Packet loader finished prematurely: %s\n", e.what());
    }

    LOG(stderr, "Finished loading.\n");

    rfs.close();
    delete handle;
  }

 private:
  unsigned char data[HDR_SIZE];

  uint64_t packet_rate_;
  uint64_t num_packets_;
  log_store *logstore_;
};

#endif /* RAMCLOUDBENCHMARK_H_ */

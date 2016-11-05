#include "packetstore.h"

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
#include <pthread.h>

#include "cpu_utilization.h"

using namespace ::netplay;
using namespace ::slog;
using namespace ::std::chrono;

#ifdef NO_LOG
#define LOG(out, fmt, ...)
#else
#define LOG(out, fmt, ...) fprintf(out, fmt, ##__VA_ARGS__)
#endif

#define MEASURE_LATENCY

const char* usage =
    "Usage: %s -n [numthreads] -r [ratelimit] -t [maxtime] [data] [attrs]\n";

typedef int64_t timestamp_t;

static timestamp_t get_timestamp() {
  struct timeval now;
  gettimeofday(&now, NULL);

  return now.tv_usec + (timestamp_t) now.tv_sec * 1000000;
}

class rate_limiter {
 public:
  rate_limiter(uint64_t ops_per_sec, packet_store::handle* handle) {
    handle_ = handle;
    min_ns_per_10000_ops = 1e13 / ops_per_sec;
    local_ops_ = 0;
    last_ts_ = high_resolution_clock::now();
    tspec_.tv_sec = 0;
    LOG(stderr, "10000 ops per %lld ns.\n", min_ns_per_10000_ops);
  }

  uint64_t insert_packet(unsigned char* data, uint16_t len, token_list& tkns) {
    uint64_t num_pkts = handle_->insert(data, len, tkns) + 1;
    local_ops_++;
    if (local_ops_ % 10000 == 0) {
      high_resolution_clock::time_point now = high_resolution_clock::now();
      auto ns_last_10000_ops =
          duration_cast<nanoseconds>(now - last_ts_).count();
      if (ns_last_10000_ops < min_ns_per_10000_ops) {
        tspec_.tv_nsec = (min_ns_per_10000_ops - ns_last_10000_ops);
        nanosleep(&tspec_, NULL);
      }
      last_ts_ = high_resolution_clock::now();
    }
    return num_pkts;
  }

  uint64_t local_ops() {
    return local_ops_;
  }

 private:
  struct timespec tspec_;
  high_resolution_clock::time_point last_ts_;
  uint64_t local_ops_;
  int64_t min_ns_per_10000_ops;
  packet_store::handle* handle_;
};

class rate_limiter_inf {
 public:
  rate_limiter_inf(uint64_t ops_per_sec, packet_store::handle* handle) {
    handle_ = handle;
    local_ops_ = 0;
    local_ops_++;
  }

  uint64_t insert_packet(unsigned char* data, uint16_t len, token_list& tkns) {
    local_ops_++;
    return handle_->insert(data, len, tkns) + 1;
  }

  uint64_t local_ops() {
    return local_ops_;
  }

 private:
  uint64_t local_ops_;
  packet_store::handle* handle_;
};

template<class rlimiter = rate_limiter_inf>
class packet_loader {
 public:
  static const uint64_t kReportRecordInterval = 11111;
  static const uint64_t kMaxNumPkts = 60 * 1e6;

  packet_loader(std::string& data_path, std::string& attr_path) {
    char resolved_path[100];
    realpath(data_path.c_str(), resolved_path);
    data_path_ = std::string(resolved_path);
    realpath(attr_path.c_str(), resolved_path);
    attr_path_ = std::string(resolved_path);

    store_ = new packet_store();

    LOG(stderr, "Loading data...\n");
    load_data();

    LOG(stderr, "Initialization complete.\n");
  }

  void init_tokens(token_list& tokens, packet_store::handle* handle) {
    handle->add_src_ip(tokens, 0);
    handle->add_dst_ip(tokens, 0);
    handle->add_src_port(tokens, 0);
    handle->add_dst_port(tokens, 0);
    handle->add_timestamp(tokens, 0);
  }

  void set_tokens(token_list& tokens, uint64_t idx) {
    tokens[0].update_data(srcips_[idx]);
    tokens[1].update_data(dstips_[idx]);
    tokens[2].update_data(sports_[idx]);
    tokens[3].update_data(dports_[idx]);
    tokens[4].update_data(timestamps_[idx]);
  }

  uint32_t parse_ip(std::string& ip) {
    uint32_t byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0;
    sscanf(ip.c_str(), "%u.%u.%u.%u", &byte0, &byte1, &byte2, &byte3);
    return byte3 | byte2 << 8 | byte1 << 16 | byte0 << 24;
  }

  uint32_t parse_time(uint32_t time) {
    unsigned char* timearr = (unsigned char*) (&time);
    return timearr[3] | timearr[2] << 8 | timearr[1] << 16 | timearr[0] << 24;
  }

  uint16_t parse_port(uint16_t port) {
    unsigned char* portarr = (unsigned char*) (&port);
    return portarr[1] | portarr[0] << 8;
  }

  void load_data() {
    std::ifstream ind(data_path_);
    std::ifstream ina(attr_path_);
    std::string attr_line;
    LOG(stderr, "Reading from path data=%s, attr=%s\n", data_path_.c_str(),
        attr_path_.c_str());

    while (std::getline(ina, attr_line) && timestamps_.size() < kMaxNumPkts) {
      std::stringstream attr_stream(attr_line);
      uint32_t ts;
      std::string srcip, dstip;
      uint16_t sport, dport;
      uint16_t len;
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
    }

    LOG(stderr, "Loaded %zu packets from dump.\n", datas_.size());
  }

  // Throughput benchmarks
  void load_packets(const uint32_t num_threads, const uint64_t timebound,
                    const uint64_t rate_limit) {

    std::vector<std::thread> workers;
    uint64_t thread_ops = timestamps_.size() / num_threads;
    uint64_t worker_rate_limit = rate_limit / num_threads;

    LOG(stderr, "Setting timebound to %llu us\n", timebound);
    for (uint32_t i = 0; i < num_threads; i++) {
      workers.push_back(
          std::move(
              std::thread([i, timebound, worker_rate_limit, thread_ops, this] {
                uint64_t idx = thread_ops * i;
                packet_store::handle* handle = store_->get_handle();
                token_list tokens;
                init_tokens(tokens, handle);
                rlimiter* limiter = new rlimiter(worker_rate_limit, handle);
                double throughput = 0;
                LOG(stderr, "Starting benchmark.\n");
                try {
                  uint64_t total_ops = 0;
                  timestamp_t start = get_timestamp();
                  while (limiter->local_ops() < thread_ops &&
                      get_timestamp() - start < timebound) {
                    set_tokens(tokens, idx);
                    total_ops = limiter->insert_packet(datas_[idx],
                        datalens_[idx], tokens);
                    idx++;
                  }
                  timestamp_t end = get_timestamp();
                  double totsecs = (double) (end - start) / (1000.0 * 1000.0);
                  throughput = ((double) limiter->local_ops() / totsecs);
                  LOG(stderr, "Thread #%u(%lfs): Throughput: %lf.\n",
                      i, totsecs, throughput);
                } catch (std::exception &e) {
                  LOG(stderr, "Throughput thread ended prematurely.\n");
                }

                std::ofstream ofs("write_throughput_" + std::to_string(i));
                ofs << throughput << "\n";
                ofs.close();
                delete limiter;
                delete handle;
              })));
    }

#ifdef MEASURE_CPU
    std::thread cpu_measure_thread([&] {
          timestamp_t start = get_timestamp();
          std::ofstream util_stream("cpu_utilization");
          cpu_utilization util;
          while (get_timestamp() - start < timebound) {
            sleep(1);
            util_stream << util.current() << "\n";
          }
          util_stream.close();
        });
#endif

    for (auto& th : workers) {
      th.join();
    }

#ifdef MEASURE_CPU
    cpu_measure_thread.join();
#endif

  }

 private:
  std::string data_path_;
  std::string attr_path_;

  std::vector<uint32_t> timestamps_;
  std::vector<uint32_t> srcips_;
  std::vector<uint32_t> dstips_;
  std::vector<uint16_t> sports_;
  std::vector<uint16_t> dports_;
  std::vector<unsigned char*> datas_;
  std::vector<uint16_t> datalens_;

  packet_store *store_;
};

void print_usage(char *exec) {
  fprintf(stderr, usage, exec);
}

int main(int argc, char** argv) {
  if (argc < 3 || argc > 9) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  int num_threads = 1;
  uint8_t num_attributes = 1;
  uint64_t timebound = UINT64_MAX;
  uint64_t rate_limit = 0;
  while ((c = getopt(argc, argv, "t:n:r:")) != -1) {
    switch (c) {
      case 't':
        timebound = atol(optarg) * 1000000;
        break;
      case 'n':
        num_threads = atoi(optarg);
        break;
      case 'r':
        rate_limit = atoll(optarg);
        break;
      default:
        fprintf(stderr, "Could not parse command line arguments.\n");
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string data_path = std::string(argv[optind]);
  std::string attr_path = std::string(argv[optind + 1]);

  if (rate_limit == 0) {
    packet_loader<> loader(data_path, attr_path);
    loader.load_packets(num_threads, timebound, 0);
  } else {
    packet_loader<rate_limiter> loader(data_path, attr_path);
    loader.load_packets(num_threads, timebound, rate_limit);
  }

  return 0;

}

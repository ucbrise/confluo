#include "packet_loader.h"

#include <unistd.h>

#ifdef NO_LOG
#define LOG(out, fmt, ...)
#else
#define LOG(out, fmt, ...) fprintf(out, fmt, ##__VA_ARGS__)
#endif

packet_loader::packet_loader(std::string& data_path, std::string& attr_path,
                             std::string& hostname) {
  char resolved_path[100];
  realpath(data_path.c_str(), resolved_path);
  data_path_ = std::string(resolved_path);
  realpath(attr_path.c_str(), resolved_path);
  attr_path_ = std::string(resolved_path);
  hostname_ = hostname;

  logstore_ = new log_store<UINT_MAX>();

  LOG(stderr, "Loading data...\n");
  load_data();

  LOG(stderr, "Initialization complete.\n");
}

void packet_loader::load_data() {
  std::ifstream ind(data_path_);
  std::ifstream ina(attr_path_);
  std::string attr_line;
  LOG(stderr, "Reading from path data=%s, attr=%s\n", data_path_.c_str(),
      attr_path_.c_str());

  while (std::getline(ina, attr_line)) {
    std::stringstream attr_stream(attr_line);
    uint32_t ts;
    std::string srcip, dstip;
    uint16_t sport, dport;
    uint16_t len;
    attr_stream >> ts >> len >> srcip >> dstip >> sport >> dport;
    unsigned char* data = new unsigned char[len];
    ind.read((char*) data, len);
    timestamps_.push_back(ts);
    srcips_.push_back(parse_ip(srcip));
    dstips_.push_back(parse_ip(srcip));
    sports_.push_back(sport);
    dports_.push_back(dport);
    datas_.push_back(data);
    datalens_.push_back(len);
  }

  LOG(stderr, "Loaded %zu packets.\n", datas_.size());
}

void packet_loader::load_packets(const uint32_t num_threads,
                                 const uint64_t timebound) {

  std::vector<std::thread> threads;

  std::ofstream rfs;
  rfs.open("record_progress");

  std::mutex report_mtx;
  uint64_t thread_ops = timestamps_.size() / num_threads;
  for (uint32_t i = 0; i < num_threads; i++) {
    threads.push_back(
        std::move(
            std::thread(
                [i, timebound, thread_ops, &rfs, &report_mtx, this] {
                  uint64_t idx = thread_ops * i;
                  double throughput = 0;
                  LOG(stderr, "Starting benchmark.\n");

                  try {
                    int64_t local_ops = 0;
                    int64_t total_ops = 0;

                    timestamp_t start = get_timestamp();
                    while (idx < thread_ops && get_timestamp() - start < timebound) {
                      total_ops = insert_packet(idx++);
                      local_ops++;
                      if (total_ops % kReportRecordInterval == 0) {
                        std::lock_guard<std::mutex> lock(report_mtx);
                        rfs << get_timestamp() << "\t" << total_ops << "\n";
                      }
                    }
                    timestamp_t end = get_timestamp();
                    double totsecs = (double) (end - start) / (1000.0 * 1000.0);
                    throughput = ((double) local_ops / totsecs);
                    LOG(stderr, "Thread #%u finished in %lf s. Throughput: %lf.\n", totsecs, throughput);
                  } catch (std::exception &e) {
                    LOG(stderr, "Throughput thread ended prematurely.\n");
                  }

                  LOG(stderr, "Throughput: %lf\n", throughput);

                  std::ofstream ofs("write_throughput", std::ofstream::out | std::ofstream::app);
                  ofs << throughput << "\n";
                  ofs.close();
                })));
  }

  for (auto& th : threads) {
    th.join();
  }

}

void PrintUsage(char *exec) {
  LOG(stderr, "Usage: %s -h [hostname] -n [numthreads] [data] [attrs]\n", exec);
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
  if (argc < 3 || argc > 9) {
    PrintUsage(argv[0]);
    return -1;
  }

  int c;
  std::string bench_type = "latency-get", hostname = "localhost";
  int num_clients = 1;
  uint8_t num_attributes = 1;
  uint64_t timebound = UINT64_MAX;
  while ((c = getopt(argc, argv, "t:n:h:")) != -1) {
    switch (c) {
      case 't':
        timebound = atol(optarg) * 10e6;
        break;
      case 'n':
        num_clients = atoi(optarg);
        break;
      case 'h':
        hostname = std::string(optarg);
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
  std::string attr_path = std::string(argv[optind + 1]);

  packet_loader loader(data_path, attr_path, hostname);
  loader.load_packets(num_clients, timebound);

  return 0;
}

#include "packet_loader.h"

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
  if (argc < 3 || argc > 11) {
    PrintUsage(argv[0]);
    return -1;
  }

  int c;
  std::string bench_type = "latency-get", hostname = "localhost";
  int num_clients = 1;
  uint8_t num_attributes = 1;
  uint64_t timebound = UINT64_MAX;
  uint64_t rate_limit = 0;
  while ((c = getopt(argc, argv, "t:n:h:r:")) != -1) {
    switch (c) {
      case 't':
        timebound = atol(optarg) * 1000000;
        break;
      case 'n':
        num_clients = atoi(optarg);
        break;
      case 'h':
        hostname = std::string(optarg);
        break;
      case 'r':
        rate_limit = atoll(optarg);
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

  if (rate_limit == 0) {
    packet_loader<> loader(data_path, attr_path, hostname);
    loader.load_packets(num_clients, timebound, 0);
  } else {
    packet_loader<rate_limiter> loader(data_path, attr_path, hostname);
    loader.load_packets(num_clients, timebound, rate_limit);
  }

  return 0;
}

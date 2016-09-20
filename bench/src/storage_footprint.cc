#include "storage_footprint.h"

void print_usage(char *exec) {
  LOG(stderr, "Usage: %s -n num_packets -r packet_rate\n", exec);
}

int main(int argc, char** argv) {
  if (argc > 5) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint64_t num_packets = 500000000ULL;
  uint64_t packet_rate = 833333ULL;
  while ((c = getopt(argc, argv, "n:r:")) != -1) {
    switch (c) {
      case 'n':
        num_packets = atoll(optarg);
        break;
      case 'r':
        packet_rate = atoll(optarg);
        break;
      default:
        LOG(stderr, "Could not parse command line arguments.\n");
    }
  }

  storage_footprint loader(packet_rate, num_packets);
  loader.load_packets();

  return 0;
}

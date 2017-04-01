#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
#include <cstdint>

struct dat {
  uint64_t ts;
  double v;
};

template<typename Out>
void split(const std::string &s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

int main(int argc, char** argv) {
  static_assert(sizeof(dat) == 16, "Data size != 16");
  assert(argc == 3);
  std::ifstream in(std::string(argv[1]).c_str());
  std::ofstream out(std::string(argv[2]).c_str());
  std::string line;
  dat d;
  while (std::getline(in, line)) {
    auto elems = split(line, ',');
    d.ts = std::stoull(elems[0]);
    d.v = std::stod(elems[1]);
    out.write(static_cast<const char*>(&d), sizeof(dat));
  }
  in.close();
  out.close();
}


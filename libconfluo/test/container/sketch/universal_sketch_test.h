#ifndef TEST_UNIVERSAL_SKETCH_TEST_H
#define TEST_UNIVERSAL_SKETCH_TEST_H

#include "container/sketch/confluo_universal_sketch.h"
#include "container/sketch/priority_queue.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class UniversalSketchTest : public testing::Test {
public:
  static int64_t f0(int64_t x) {
    return 1;
  }

  static int64_t f1(int64_t x) {
    return int64_t(x);
  }

  static int64_t f2(int64_t x) {
    return int64_t(x * x);
  }

//  static int64_t update(universal_sketch<int>& us, std::map<int, uint64_t>& hist) {
//    std::function<int64_t(int64_t)> g = f2;
//    int64_t actual = 0;
//
//    double avg_lat = 0;
//
//    LOG_INFO << "Creating stream...";
//    std::vector<int> stream;
//    for (auto p : hist) {
//      actual += g(p.second);
//      stream.push_back(p.first);
//      if (stream.size() % 1000 == 0) {
//        LOG_INFO << stream.size() << ", avg_lat = " << (avg_lat)/1000;
//        avg_lat = 0;
//      }
//      if (p.second > 1) {
//        int64_t a = time_utils::cur_ns();
//        us.update(p.first, p.second - 1);
//        int64_t b = time_utils::cur_ns();
//        avg_lat += (b - a);
//      }
//    }
//
//    LOG_INFO << "Shuffling...";
//    std::random_shuffle(stream.begin(), stream.end());
//
//    LOG_INFO << "Updating from shuffled...";
//    for (auto elem : stream) {
//      us.update(elem);
//    }
//
//    return actual;
//  }
//
//  static void eval(universal_sketch<int>& us, int64_t actual, size_t layers, std::ofstream& out) {
//
//    std::function<int64_t(int64_t)> g = f2;
//
//    for (size_t l = 1; l < layers; l++) {
//      int64_t eval_a = time_utils::cur_ns();
//      int64_t g_sum = us.evaluate(g, l);
//      int64_t eval_b = time_utils::cur_ns();
//      double error = double(std::abs(g_sum - actual))/actual;
//      LOG_INFO << "Layers: " << l;
//      LOG_INFO << g_sum << " vs " << actual;
//      LOG_INFO << "Error: " << error;
//      LOG_INFO << "";
//      out << l << " " << us.storage_size()/1024 << " " << error << "\n";
//    }
//
//  }

};

TEST_F(UniversalSketchTest, EstimateAccuracyTest) {

  std::ifstream in("/Users/Ujval/dev/research/confluo/analyze/normal-0u-1000sd.hist");
  LOG_INFO << "Loading...";
  std::map<int, uint64_t> hist;
  std::string line;

  while (getline(in, line)) {
    size_t space_idx = line.find(' ');
    int key = std::stoi(line.substr(0, space_idx));
    uint64_t freq = std::stoull(line.substr(space_idx + 1));
    hist[key] = uint32_t(freq);
  }

  // 0.01 margin of error
  size_t l = 32;
  size_t t = 32;
  size_t b = 2700; // epsilon = 0.01
  size_t k = 32;

  // Update universal sketch
//  universal_sketch<int> us(l, b, t, k);
//  update(us, hist);
//
//  // Update actual heavy hitters
//  pq<int, int64_t> hhs_actual;
//  for (auto p : hist) {
//    if (hhs_actual.size() < k) {
//      hhs_actual.pushp(p.first, p.second);
//    } else {
//      if (hhs_actual.top().priority < p.second) {
//        hhs_actual.pop();
//        hhs_actual.pushp(p.first, p.second);
//      }
//    }
//  }
//
//  for (auto p : hhs_actual) {
//    auto est = us.estimate_count(p.key);
//    auto error = std::abs(est - p.priority) * 1.0 / est;
//    LOG_INFO << "Error for " << p.key << ": " << error << " (" << est << " vs " << p.priority << ")";
//  }

}


#endif //TEST_UNIVERSAL_SKETCH_TEST_H

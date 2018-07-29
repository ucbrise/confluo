#ifndef TEST_UNIVERSAL_SKETCH_TEST_H
#define TEST_UNIVERSAL_SKETCH_TEST_H

#include "container/sketch/universal_sketch.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class UniversalSketchTest : public testing::Test {
public:
  static constexpr int N = int(5e8);

  static constexpr int MEAN = 0;
  static constexpr int SD = 100;

  static constexpr int X0 = 1;
  static constexpr int X1 = int(1e8);

  UniversalSketchTest()
          : dist(MEAN, SD),
            udist(0.0, 1.0) {
    std::random_device rd;
    e2 = std::mt19937(rd());
  }

  int sample_zipf(double alpha = 1.5) {
    static bool first = true;      // Static first time flag
    static double c = 0;          // Normalization constant
    static double* sum_probs;     // Pre-calculated sum of probabilities
    double z = 0.0;               // Uniform random number (0 < z < 1)
    int zipf_value = 0;           // Computed exponential value to be returned
    int i;                        // Loop counter
    int low, high, mid;           // Binary-search bounds

    // Compute normalization constant on first call only
    if (first) {
      for (i = X0; i <= X1; i++)
        c = c + (1.0 / pow((double) i, alpha));
      c = 1.0 / c;
      sum_probs = new double[X1+1];
      sum_probs[0] = 0;
      for (i = X0; i <= X1; i++)
        sum_probs[i] = sum_probs[i-1] + c / pow((double) i, alpha);
      first = false;
    }

    while ((z == 0) || (z == 1)) // (0 < z < 1)
      z = udist(e2);

    low = X0, high = X1, mid = 0;
    while (low <= high) {
      mid = std::floor((low+high)/2);
      if (sum_probs[mid] >= z && sum_probs[mid-1] < z) {
        zipf_value = mid;
        break;
      }
      else if (sum_probs[mid] >= z) {
        high = mid-1;
      }
      else {
        low = mid+1;
      }
    }
    assert((zipf_value >= X0) && (zipf_value <= X1));
    return zipf_value;
  }

  double generate_zipf(std::map<int, long int>& hist) {
    for (int n = 0; n < N; n++) {
      hist[sample_zipf(1.2)]++;
      if (n % int(1e5) == 0)
        LOG_INFO << "Completed... " << n;
    }

    double l2_sq = 0.0;
    for (auto p : hist)
      l2_sq += (p.second * p.second);

    return std::sqrt(l2_sq);
  }

  static int64_t f0(int64_t x) {
    return 1;
  }

  static int64_t f1(int64_t x) {
    return int64_t(x);
  }

  static int64_t f2(int64_t x) {
    return int64_t(x * x);
  }

  static int64_t update(universal_sketch<int>& us, std::map<int, uint64_t>& hist) {
    std::function<int64_t(int64_t)> g = f2;
    int64_t actual = 0;

    LOG_INFO << "Creating stream...";
    std::vector<int> stream;
    for (auto p : hist) {
      actual += g(p.second);
      stream.push_back(p.first);
      if (p.second > 1) {
        us.update(p.first, p.second);
      }
    }

    LOG_INFO << "Shuffling...";
    std::random_shuffle(stream.begin(), stream.end());

    LOG_INFO << "Updating from shuffled...";
    for (auto elem : stream) {
      us.update(elem);
    }

    int64_t update_a = time_utils::cur_ns();
    int64_t update_b = time_utils::cur_ns();

    return actual;
  }

  static void eval(universal_sketch<int>& us, int64_t actual, size_t layers, std::ofstream& out) {

    std::function<int64_t(int64_t)> g = f2;

    for (size_t l = 1; l < layers; l++) {
      int64_t eval_a = time_utils::cur_ns();
      int64_t g_sum = us.evaluate(g, l);
      int64_t eval_b = time_utils::cur_ns();
      double error = double(std::abs(g_sum - actual))/actual;

      LOG_INFO << "Layers: " << l;
      LOG_INFO << g_sum << " vs " << actual;
//      LOG_INFO << "Storage size: " << us.storage_size()/1024 << " KB";
      LOG_INFO << "Error: " << error;
      //LOG_INFO << "Latency per update: " << (update_b - update_a)/1000 << " us";
//      LOG_INFO << "Evaluation latency: " << (eval_b - eval_a)/1000 << " us";
      LOG_INFO << "";

      out << l << " " << us.storage_size()/1024 << " " << error << "\n";
    }

  }

  std::normal_distribution<double> dist;
  std::uniform_real_distribution<> udist;
  std::mt19937 e2;

};

TEST_F(UniversalSketchTest, GenerateTest) {

//  std::ofstream out("/Users/Ujval/dev/research/confluo/analyze/zipf.hist");
//  double l2 = generate_zipf(hist);

//  std::ofstream out("/Users/Ujval/dev/research/confluo/analyze/normal.hist");
//  std::map<int, long int> hist;
//  for (size_t i = 0; i < N; i++)
//    hist[dist(e2)]++;

//  for (auto p: hist)
//    out << p.first << " " << p.second << "\n";

}

//TEST_F(UniversalSketchTest, EstimateAccuracyTest) {

//  std::ifstream in("/Users/Ujval/dev/research/confluo/analyze/zipf.hist");
////  std::ifstream in("/Users/Ujval/dev/research/confluo/analyze/normal.hist");

////  std::ofstream out("/Users/Ujval/dev/research/confluo/univ_sketch_error.001.out");
//  std::ofstream out("/Users/Ujval/dev/research/confluo/univ_sketch_error.005.out");
////  std::ofstream out("/Users/Ujval/dev/research/confluo/univ_sketch_error.03.out");

//  LOG_INFO << "Loading...";
//  std::map<int, uint64_t> hist;
//  std::string line;

//  while (getline(in, line)) {
//    size_t space_idx = line.find(" ");
//    int key = std::stoi(line.substr(0, space_idx));
//    uint64_t freq = std::stoull(line.substr(space_idx + 1));
//    hist[key] = uint32_t(freq);
//  }

//  // 0.01 margin of error
//  size_t l = 32;
//  size_t t = 64;
////  size_t b = 27000; // epsilon = 0.01
////  size_t b = 1087; // epsilon = 0.05
//  size_t b = 50; // epsilon = idk
//  size_t k = 20;
//  double a = 0;
////  universal_sketch<int> us(l, t, b, k, a);
////  eval(us, hist, l, out);

//  // 0.05 margin of error
//  universal_sketch<int> us(l, t, b, k, a);

//  LOG_INFO << "Updating...";
//  int64_t actual = update(us, hist);
//  LOG_INFO << "Evaluating...";
//  eval(us, actual, l, out);

//}

const int UniversalSketchTest::N;

#endif //TEST_UNIVERSAL_SKETCH_TEST_H

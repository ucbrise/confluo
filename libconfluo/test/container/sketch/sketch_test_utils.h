#ifndef CONFLUO_SKETCH_TEST_UTILS_H
#define CONFLUO_SKETCH_TEST_UTILS_H

#include <cassert>
#include <rand_utils.h>
#include <unordered_map>

class ZipfGenerator {
 public:
  static constexpr int X0 = 1;
  static constexpr int X1 = int(1e8);

  ZipfGenerator()
      : udist(0.0, 1.0) {
    std::random_device rd;
    e2 = std::mt19937(rd());
  }

  int sample_zipf(double alpha) {
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

  double generate_zipf(std::unordered_map<int, uint64_t>& hist, int n) {
    for (int i = 0; i < n; i++) {
      hist[sample_zipf(1.2)]++;
    }
    double l2_sq = 0.0;
    for (auto p : hist)
      l2_sq += (p.second * p.second);
    return std::sqrt(l2_sq);
  }

  std::uniform_real_distribution<> udist;
  std::mt19937 e2;

};

constexpr int ZipfGenerator::X0;
constexpr int ZipfGenerator::X1;

#endif //CONFLUO_SKETCH_TEST_UTILS_H

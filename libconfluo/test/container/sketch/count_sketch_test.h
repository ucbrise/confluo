#ifndef TEST_COUNT_SKETCH_TEST_H_
#define TEST_COUNT_SKETCH_TEST_H_

#include <functional>

#include "/Users/Ujval/dev/research/univmon_extension/simulator/V0.3/countSketch.h"
#include "container/sketch/count_sketch.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class CountSketchTest : public testing::Test {
 public:
  static constexpr int N = 1000000;

  static constexpr double MEAN = 0.0;
  static constexpr double SD = 500.0;

  static constexpr int X0 = 1;
  static constexpr int X1 = 1000000;

  CountSketchTest()
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
    return(zipf_value);
  }

  double generate_zipf(std::map<int, long int>& hist) {
    for (int n = 0; n < N; n++)
      hist[std::abs(sample_zipf(1.2))]++;

    double l2_sq = 0.0;
    for (auto p : hist)
      l2_sq += (p.second * p.second);

    return std::sqrt(l2_sq);
  }

 private:
  std::normal_distribution<double> dist;
  std::uniform_real_distribution<> udist;
  std::mt19937 e2;

};

const int CountSketchTest::N;

TEST_F(CountSketchTest, EstimateAccuracyTest) {

  double alpha = 0.01;
  double epsilon[] = { 0.01, 0.02, 0.04, 0.08, 0.16 };
  double gamma[] = { 0.01 };

  std::map<int, long int> hist;
  double l2 = generate_zipf(hist);
  std::ofstream out("sketch_error.out");
  LOG_INFO << "L2-norm: " << l2;

  for (double g : gamma) {
    for (double e : epsilon) {

      auto cs = count_sketch<int>::create_parameterized(g, e);

      for (auto p : hist)
        for (int i = 0; i < p.second; i++)
          cs.update(p.first);

      double num_measurements = 0;
      std::vector<double> errors;
      std::vector<double> relevant_errors;

      size_t negative_count = 0;
      for (auto p : hist) {
        int64_t actual = p.second;
        num_measurements++;

        // My estimate
        int64_t est = cs.estimate(p.first);
        double diff = std::abs(est - actual);
        double error = diff/actual;
        errors.push_back(error);

        if (actual > alpha * l2) {
          relevant_errors.push_back(error);
        }

        if (est < 0) {
          negative_count++;
        }
      }

      // Compute overall stats
      std::sort(errors.begin(), errors.end());
      double avg_err = std::accumulate(errors.begin(), errors.end(), 0.0)/errors.size();

      std::sort(relevant_errors.begin(), relevant_errors.end());
      double avg_relv_err = std::accumulate(relevant_errors.begin(), relevant_errors.end(), 0.0)/relevant_errors.size();

      // Print
      LOG_INFO << "Dimensions: " << cs.depth() << " x " << cs.width();
      LOG_INFO << "Sketch size: " << (cs.storage_size() / 1024) << " KB";
      LOG_INFO << "Negative rate: " << negative_count/num_measurements;
      LOG_INFO << "Median error: " << errors[int(errors.size()/2)];
      LOG_INFO << "Median relevant error: " << relevant_errors[int(relevant_errors.size()/2)];
      LOG_INFO << "Mean error: " << avg_err;
      LOG_INFO << "Mean relevant error: " << avg_relv_err;
      LOG_INFO << "";

      ASSERT_LT(avg_relv_err, e);

//      summary_out << num_rows[i] << " " << counters_per_row[j] << " " << violator_count/num_measurements <<
//       " " << errors[int(errors.size()/2)] << "\n";

    }
  }
}

#endif /* TEST_COUNT_SKETCH_TEST */

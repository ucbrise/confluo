#ifndef TEST_CONTAINER_SKETCH_COUNT_MIN_SKETCH_TEST_H_
#define TEST_CONTAINER_SKETCH_COUNT_MIN_SKETCH_TEST_H_

#include <functional>

#include "/Users/Ujval/dev/research/univmon_extension/simulator/V0.3/countSketch.h"
#include "container/sketch/count_sketch.h"
#include "container/sketch/universal_monitor.h"
#include "gtest/gtest.h"

using namespace ::confluo::sketch;

class CountSketchTest : public testing::Test {
 public:
  static const int N = 1000;
};

const int CountSketchTest::N;

TEST_F(CountSketchTest, EstimateAccuracyTest) {
  double epsilon = 0.0;

  std::ofstream out("sketch_error.out");

  std::random_device rd;
  std::mt19937 e2(rd());
  std::normal_distribution<double> dist(0, 1000);
  std::map<int, long int> hist;
  for (int n = 0; n < 10000; n++) {
    hist[std::abs(std::round(dist(e2)))]++;
  }

  size_t num_rows[] = { 4, 8, 16, 32 };
  size_t counters_per_row[] = {  8000 };

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 1; j++) {

      count_sketch<int> cs(num_rows[i], counters_per_row[j]);
      CountSketch alan_cs(counters_per_row[j], num_rows[i], 10000);

      for (auto p : hist) {
        for (int i = 0; i < p.second; i++) {
          cs.update(p.first);
          alan_cs.add(p.first);
        }
      }

      double num_measurements = 0;

      double violator_count = 0;
      std::vector<double> errors;
      std::vector<double> alan_errors;

      size_t negative_count = 0;
      for (auto p : hist) {
        int64_t actual = p.second;
        num_measurements++;

        // My estimate
        int64_t est = cs.estimate(p.first);
        double diff = std::abs(est - actual);
        double error = diff/actual;
        errors.push_back(error);

        //if (error > epsilon) {
        //  violator_count++;
        //}
        if (est < 0) {
          negative_count++;
        }

        // Alan estimate
        int alan_est = alan_cs.estimate(p.first);
        double alan_diff = std::abs(alan_est - actual);
        double alan_error = alan_diff/actual;
        alan_errors.push_back(alan_error);

      }

      // Compute overall stats
      std::sort(errors.begin(), errors.end());
      double average = std::accumulate(errors.begin(), errors.end(), 0.0)/errors.size();

      std::sort(alan_errors.begin(), alan_errors.end());
      double alan_average = std::accumulate(alan_errors.begin(), alan_errors.end(), 0.0)/alan_errors.size();

      // Print
      LOG_INFO << "Dimensions: " << num_rows[i] << " x " << counters_per_row[j];
      LOG_INFO << "Sketch size: " << cs.storage_size() << "B";
      LOG_INFO << "Negative rate: " << negative_count/num_measurements;
      LOG_INFO << "Median error: " << errors[int(errors.size()/2)];
     // LOG_INFO << "Median error (alan): " << alan_errors[int(alan_errors.size()/2)] << "\n";

      //LOG_INFO<< "Incorrectness rate: " << violator_count/num_measurements;
      //LOG_INFO << "Mean error: " << average << "\n";
      //std::cout << "Failure rate: " << violator_count/num_measurements << "\n";
      //summary_out << num_rows[i] << " " << counters_per_row[j] << " " << violator_count/num_measurements <<
      // " " << errors[int(errors.size()/2)] << "\n";

    }
  }
}

#endif /* TEST_CONTAINER_SKETCH_COUNT_MIN_SKETCH_TEST_H_ */

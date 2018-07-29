#include <algorithm>
#include <array>
#include <assert.h>
#include <assertions.h>
#include <iostream>
#include <fstream>
#include <map>
#include <random>
#include <boost/program_options.hpp>

#include "container/sketch/universal_sketch.h"

using namespace ::confluo::sketch;
namespace po = boost::program_options;

typedef universal_sketch<int> univ_sketch_t;

class generator {

 public:
  generator()
      : udist(0.0, 1.0) {
    std::random_device rd;
    e2 = std::mt19937(rd());
  }

  int sample_zipf(double alpha, int x0, int x1) {
    static bool first = true;      // Static first time flag
    static double c = 0;          // Normalization constant
    static double* sum_probs;     // Pre-calculated sum of probabilities
    double z = 0.0;               // Uniform random number (0 < z < 1)
    int zipf_value = 0;           // Computed exponential value to be returned
    int i;                        // Loop counter
    int low, high, mid;           // Binary-search bounds

    // Compute normalization constant on first call only
    if (first) {
      for (i = x0; i <= x1; i++)
        c = c + (1.0 / pow((double) i, alpha));
      c = 1.0 / c;
      sum_probs = new double[x1+1];
      sum_probs[0] = 0;
      for (i = x0; i <= x1; i++)
        sum_probs[i] = sum_probs[i-1] + c / pow((double) i, alpha);
      first = false;
    }

    while ((z == 0) || (z == 1)) // (0 < z < 1)
      z = udist(e2);

    low = x0, high = x1, mid = 0;
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
    assert((zipf_value >= x0) && (zipf_value <= x1));
    return zipf_value;
  }

  double generate_zipf(std::map<int, long int>& hist, int n, int x0, int x1) {
    for (int i = 0; i < n; i++) {
      hist[sample_zipf(1.2, x0, x1)]++;
      if (n % int(1e5) == 0)
        std::cout << "Completed... " << i << "\n";
    }

    double l2_sq = 0.0;
    for (auto p : hist)
      l2_sq += (p.second * p.second);

    return std::sqrt(l2_sq);
  }

  double generate_gaussian(std::map<int, long int>& hist, int n, int mean, int sd) {
    std::normal_distribution<double> dist;
    for (int i = 0; i < n; i++) {
      hist[dist(e2)]++;
    }

    double l2_sq = 0.0;
    for (auto p : hist)
      l2_sq += (p.second * p.second);

    return std::sqrt(l2_sq);
  }

  std::uniform_real_distribution<> udist;
  std::mt19937 e2;

};

class experiment {

 public:
  experiment() = default;

  static int64_t f0(int64_t x) {
    return 1;
  }

  static int64_t f1(int64_t x) {
    return int64_t(x);
  }

  static int64_t f2(int64_t x) {
    return int64_t(x * x);
  }

  static int64_t update(univ_sketch_t& us, std::map<int, uint64_t>& hist) {
    std::function<int64_t(int64_t)> g = f2;
    int64_t actual = 0;

    std::cout << "Updating n-1...\n";
    std::vector<int> stream;
    for (auto p : hist) {
      actual += g(p.second);
      stream.push_back(p.first);
      if (p.second > 1) {
        us.update(p.first, p.second);
      }
    }

    std::random_shuffle(stream.begin(), stream.end());
    std::cout << "Updating last...\n";
    for (auto elem : stream) {
      us.update(elem);
    }

    return actual;
  }

  static void eval(univ_sketch_t& us, int64_t actual, size_t layers, std::ofstream& out, size_t b, size_t k) {

    std::function<int64_t(int64_t)> g = f2;

    for (size_t l = 1; l <= layers; l++) {
      int64_t g_sum = us.evaluate(g, l);
      double error = double(std::abs(g_sum - actual))/actual;

      std::cout << "(l, b, k): (" << l << " " << b << " " << k << ")\n";
      std::cout << g_sum << " vs " << actual << "\n";
      std::cout << "Error: " << error << "\n";
      std::cout << "Storage size: " << us.storage_size(l)/1024.0 << " KB" << "\n";
      std::cout << "\n";

      out << l << "," << b << "," << k << "," << us.storage_size(l)/1024.0 << "," << error << "\n";
    }

  }

};

class count_sketch_experiment {
public:
  static void run(std::map<int, uint64_t> &hist, std::ofstream &out) {
    std::array<double, 7> epsilon = {{0.005, 0.01, 0.02, 0.04, 0.06, 0.08, 0.10}};
    std::array<double, 1> gamma = {{0.01}};
//    std::array<double, 7> gamma = {0.005, 0.01, 0.02, 0.04, 0.06, 0.08, 0.10 };

    double l2 = 0;
    double l1 = 0;
    size_t k = 200;
    for (auto p : hist) {
      l2 += (p.second * p.second);
      l1 += p.second;
    }
    l2 = std::sqrt(l2);

    std::cout << "Updating...\n";
    hash_manager m1(count_sketch<int>::perror_to_depth(gamma.back()));
    hash_manager m2(count_sketch<int>::perror_to_depth(gamma.back()));
    for (double g : gamma) {
      for (double e : epsilon) {

        heavy_hitter_set<int, int64_t> hhs;
        heavy_hitter_set<int, int64_t> hhs_actual;
        auto cs = count_sketch<int>(count_sketch<int>::error_margin_to_width(e),
                                    count_sketch<int>::perror_to_depth(g),
                                    m1, m2);

        // Forward to last updates
        for (auto p : hist) {
          if (p.second > 1)
            cs.update(p.first, p.second);
        }

        for (auto p : hist) {
          cs.update(p.first);
          int64_t est = cs.estimate(p.first);
          if (hhs.size() < k) {
            hhs.pushp(p.first, est);
          } else {
            int head = hhs.top().key_;
            int64_t head_priority = hhs.top().priority_;
            if (head_priority < est) {
              hhs.pop();
              hhs.pushp(p.first, est);
              assert_throw(hhs.top().priority_ >= head_priority,
                           std::to_string(hhs.top().priority_) + " > " + std::to_string(head_priority));
            }
          }
          if (hhs_actual.size() < k) {
            hhs_actual.pushp(p.first, est);
          } else {
            if (hhs_actual.top().priority_ < est) {
              hhs_actual.pop();
              hhs_actual.pushp(p.first, p.second);
            }
          }
        }

        std::cout << "Verifying correctness...\n";
        // Verify correctness
        int64_t smallest_actual = hhs_actual.top().priority_;
        for (auto hh : hhs) {
          assert_throw((1 - e) * smallest_actual < hist[hh.key_],
                       std::to_string(smallest_actual) + " < \n" + to_string(hhs) + "\n\n" + to_string(hhs_actual) + "\n");
        }

        std::vector<int> top_k;
        while (!hhs.empty()) {
          top_k.push_back(hhs.top().key_);
          hhs.pop();
        }

        std::cout << "Computing error...\n";
        // Compute relevant error
        double l1_regularized_err = 0.0;
        double l2_regularized_err = 0.0;
        double avg_err = 0.0;
        size_t cur_k = 0;
        for (size_t i = top_k.size(); i-- > 0;) {
          size_t k_so_far = top_k.size() - i + 1;
          int64_t actual = hist[top_k[i]];
          int64_t est = cs.estimate(top_k[i]);
          double diff = std::abs(est - actual);
          double error = diff / actual;
          avg_err += error;
          out << cs.width() << "," << cs.depth() << "," << e << "," << g << ","
              << cs.storage_size() / 1024 << "," << avg_err/k_so_far << "\n";
        }

        // Print
        std::cout << "Dimensions: " << cs.depth() << " x " << cs.width() << "\n";
        std::cout << "Sketch size: " << (cs.storage_size() / 1024) << " KB" << "\n";
        std::cout << "Avg error: " << avg_err/top_k.size() << "\n";
        std::cout << "\n";

      }
    }
  }

 private:
  static std::string to_string(heavy_hitter_set<int, int64_t>& hhs) {
    std::string out = "";
    for (auto hh : hhs) {
      out += " (" + std::to_string(hh.key_) + " = " + std::to_string(hh.priority_) + "), ";
    }
    return out;
  }
};


int main(int argc, char** argv) {

  po::options_description desc("Usage");
  desc.add_options()
          ("type", po::value<std::string>()->default_value("universal"), "[count|universal]")
          ("l", po::value<size_t>()->default_value(32), "# of layers")
          ("t", po::value<size_t>()->default_value(64), "Sketch depth")
          ("b", po::value<size_t>()->default_value(27000), "Sketch width")
          ("k", po::value<size_t>()->default_value(64), "# of heavy hitters")
          ("a", po::value<double>()->default_value(0), "alpha")
          ("in", po::value<std::string>()->default_value("sketch_exp.hist"), "Histogram input")
          ("out", po::value<std::string>()->default_value("sketch_exp.csv"), "output (error or histogram)");

  po::variables_map opts;
  po::store(po::parse_command_line(argc, argv, desc), opts);
  try {
    po::notify(opts);
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  std::string in_path = opts["in"].as<std::string>();
  std::string out_path = opts["out"].as<std::string>();
  std::ifstream in(in_path);
  std::ofstream out(out_path);

  std::cout << "Loading...\n";
  std::map<int, uint64_t> hist;
  std::string line;
  while (getline(in, line)) {
    size_t space_idx = line.find(' ');
    int key = std::stoi(line.substr(0, space_idx));
    uint64_t freq = std::stoull(line.substr(space_idx + 1));
    hist[key] = uint32_t(freq);
  }

  std::string type = opts["type"].as<std::string>();
  if (type == "count") {
    count_sketch_experiment::run(hist, out);
    return 0;
  }

  size_t l = opts["l"].as<size_t>();
  size_t t = opts["t"].as<size_t>();
  size_t b = opts["b"].as<size_t>();
  size_t k = opts["k"].as<size_t>();
  double a = opts["a"].as<double>();

  size_t tt[] = {32, 36, 40, 44, 48 };
  size_t kk[] = { 4, 8, 12, 14, 16, 18, 20, 40, 60, 80, 100 };
  size_t bb[] = { 1000, 2000, 4000, 8000, 16000, 32000 };
  hash_manager m1(l);
  auto *m2 = new hash_manager[l]();
  auto *m3 = new hash_manager[l]();
  for (size_t i = 0; i < l; i++) {
    m2[i] = hash_manager(tt[4]);
    m3[i] = hash_manager(tt[4]);
  }
  auto pwih = pairwise_indep_hash::generate_random();
  for (size_t ttt: tt)
    for (size_t kkk : kk)
      for (size_t bbb : bb) {
        std::cout << "Updating...\n";
        univ_sketch_t us(l, ttt, bbb, kkk, a, m1, m2, m3, pwih);
        int64_t actual = experiment::update(us, hist);
        std::cout << "Evaluating...\n";
        experiment::eval(us, actual, l, out, bbb, kkk);
      }

}

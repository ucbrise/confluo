#ifndef DIALOG_PERIODIC_TASK_H_
#define DIALOG_PERIODIC_TASK_H_

#include <thread>
#include <chrono>

#include "logger.h"
#include "atomic.h"

class periodic_task {
 public:
  periodic_task(const std::string& name)
      : name_(name),
        enabled_(false) {
  }

  ~periodic_task() {
    stop();
  }

  bool stop() {
    bool expected = true;
    if (atomic::strong::cas(&enabled_, &expected, false)) {
      if (executor_.joinable())
        executor_.join();
      return true;
    }
    return false;
  }

  bool start(std::function<void(void)> task, uint64_t interval_ms = 1) {
    bool expected = false;
    if (atomic::strong::cas(&enabled_, &expected, true)) {
      executor_ =
          std::thread(
              [this, task, interval_ms] {
                const auto interval = std::chrono::milliseconds(interval_ms);
                while (atomic::load(&enabled_)) {
                  auto start = std::chrono::steady_clock::now();
                  task();
                  auto end = std::chrono::steady_clock::now();
                  auto elapsed = end - start;

                  if (elapsed < interval) {
                    std::this_thread::sleep_for(interval - elapsed);
                  } else {
                    auto extra_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed - interval).count();
                    LOG_WARN << name_ << ": Last execution overshot by " << extra_us << "us";
                  }
                }});
      return true;
    }
    return false;
  }

 private:
  std::string name_;
  atomic::type<bool> enabled_;
  std::thread executor_;
};

#endif /* DIALOG_PERIODIC_TASK_H_ */

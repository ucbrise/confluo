#ifndef DIALOG_PERIODIC_TASK_H_
#define DIALOG_PERIODIC_TASK_H_

#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>

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
    LOG_TRACE<< "Attempting to stop periodic_task...";
    bool expected = true;
    if (atomic::strong::cas(&enabled_, &expected, false)) {
      if (executor_.joinable())
      executor_.join();
      LOG_TRACE << "Task stopped.";
      return true;
    }
    LOG_TRACE << "Task was already stopped.";
    return false;
  }

  bool start(std::function<void(void)> task, uint64_t interval_ms = 1) {
    bool expected = false;
    if (atomic::strong::cas(&enabled_, &expected, true)) {
      std::condition_variable cv;
      std::mutex start_mtx;
      bool ready = false;
      executor_ =
      std::thread(
          [this, task, interval_ms, &ready, &start_mtx, &cv] {
            const auto interval = std::chrono::milliseconds(interval_ms);
            {
              std::lock_guard<std::mutex> lk(start_mtx);
              ready = true;
            }
            cv.notify_one();
            LOG_INFO << name_ << " task started...";
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

      std::unique_lock<std::mutex> lk(start_mtx);
      cv.wait(lk, [&ready] {return ready;});
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

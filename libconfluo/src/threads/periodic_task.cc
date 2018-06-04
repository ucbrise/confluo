#include "threads/periodic_task.h"

periodic_task::periodic_task(const std::string &name)
    : name_(name),
      enabled_(false) {
}

periodic_task::~periodic_task() {
  stop();
}

bool periodic_task::stop() {
  LOG_TRACE << "Attempting to stop periodic_task...";
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

bool periodic_task::start(std::function<void(void)> task, uint64_t interval_ms) {
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
                auto time_to_wait = interval - elapsed;
                if (time_to_wait > std::chrono::milliseconds::zero()) {
                  std::this_thread::sleep_for(time_to_wait);
                } else {
                  auto extra_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed - interval).count();
                  LOG_WARN << name_ << ": Last execution overshot by " << extra_us << "us";
                }
              }
            });

    std::unique_lock<std::mutex> lk(start_mtx);
    cv.wait(lk, [&ready] { return ready; });
    return true;
  }
  return false;
}

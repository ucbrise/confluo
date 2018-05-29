#include "threads/task_pool.h"

template<class... ARGS>
task_type::task_type(ARGS &&... args)
    : func(std::forward<ARGS>(args)...),
      next(nullptr) {
}

task_queue::task_queue() {
  valid_.store(true);
}

task_queue::~task_queue() {
  invalidate();
}

void task_queue::invalidate(void) {
  std::lock_guard<std::mutex> lock{mutex_};
  valid_ = false;
  condition_.notify_all();
}

bool task_queue::dequeue(task_queue::function_t &out) {
  std::unique_lock<std::mutex> lock(mutex_);
  condition_.wait(lock, [this]() { return !queue_.empty() || !valid_; });

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if (!valid_)
    return false;

  out = std::move(queue_.front());
  queue_.pop();
  return true;
}

bool task_queue::empty() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.empty();
}

void task_queue::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  while (!queue_.empty()) {
    queue_.pop();
  }
  condition_.notify_all();
}

task_worker::task_worker(task_queue &queue)
    : stop_(false),
      queue_(queue) {
}

task_worker::~task_worker() {
  stop();
}

void task_worker::start() {
  worker_ = std::thread([this]() {
    task_queue::function_t task;
    while (!atomic::load(&stop_)) {
      if (queue_.dequeue(task)) {
        try {
          task();
        } catch (std::exception &e) {
          LOG_ERROR << "Could not execute task: " << e.what();
          fprintf(stderr, "Exception: %s\n", e.what());
        }
      }
    }
  });
}

void task_worker::stop() {
  atomic::store(&stop_, true);
  if (worker_.joinable())
    worker_.join();
}

task_pool::task_pool(size_t num_workers) {
  for (size_t i = 0; i < num_workers; i++) {
    workers_.push_back(new task_worker(queue_));
    workers_[i]->start();
  }
}

task_pool::~task_pool() {
  queue_.invalidate();
  for (task_worker *worker : workers_) {
    delete worker;
  }
}

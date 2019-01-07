#ifndef CONFLUO_THREADS_PERIODIC_TASK_H_
#define CONFLUO_THREADS_PERIODIC_TASK_H_

#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <functional>

#include "atomic.h"
#include "logger.h"

/**
 * The periodic task class. Contains functionality to start and stop
 * the specified task.
 */
class periodic_task {
 public:
  /**
   * Constructor for periodic name that initializes the task
   * @param name The name of the task
   */
  periodic_task(const std::string &name);

  /**
   * Default destructor that stops the task
   */
  ~periodic_task();

  /**
   * Stops the periodic task
   * @return True if the task was successfully stopped, false if the
   * task was already stopped
   */
  bool stop();

  /**
   * Starts the periodic task to run
   * @param task The function that represents the work to do
   * @param interval_ms The time in between executions of the task
   * @return True if the task was executed successfully, false otherwise
   */
  bool start(std::function<void(void)> task, uint64_t interval_ms = 1);

 private:
  std::string name_;
  atomic::type<bool> enabled_;
  std::thread executor_;
};

#endif /* CONFLUO_THREADS_PERIODIC_TASK_H_ */

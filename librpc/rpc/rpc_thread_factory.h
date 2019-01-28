#ifndef CONFLUO_RPC_THREAD_FACTORY_H
#define CONFLUO_RPC_THREAD_FACTORY_H

#include <thrift/concurrency/Thread.h>
#include <thrift/stdcxx.h>
#include <thread>

namespace confluo {
namespace rpc {

class rpc_thread_factory : public ::apache::thrift::concurrency::ThreadFactory {
 public:
  /**
   * POSIX Thread scheduler policies
   */
  enum POLICY { OTHER, FIFO, ROUND_ROBIN };

  /**
   * POSIX Thread scheduler relative priorities,
   *
   * Absolute priority is determined by scheduler policy and OS. This
   * enumeration specifies relative priorities such that one can specify a
   * priority within a giving scheduler policy without knowing the absolute
   * value of the priority.
   */
  enum PRIORITY {
    LOWEST = 0,
    LOWER = 1,
    LOW = 2,
    NORMAL = 3,
    HIGH = 4,
    HIGHER = 5,
    HIGHEST = 6,
    INCREMENT = 7,
    DECREMENT = 8
  };

  explicit rpc_thread_factory(POLICY policy = ROUND_ROBIN,
                              PRIORITY priority = NORMAL,
                              int stackSize = 1,
                              bool detached = false);

  explicit rpc_thread_factory(bool detached);

  // From ThreadFactory;
  ::apache::thrift::stdcxx::shared_ptr<::apache::thrift::concurrency::Thread>
  newThread(::apache::thrift::stdcxx::shared_ptr<::apache::thrift::concurrency::Runnable> runnable) const override;

  // From ThreadFactory;
  ::apache::thrift::concurrency::Thread::id_t getCurrentThreadId() const override;

  /**
    * Gets stack size for newly created threads
    *
    * @return int size in megabytes
    */
  virtual int getStackSize() const;

  /**
   * Sets stack size for newly created threads
   *
   * @param value size in megabytes
   */
  virtual void setStackSize(int value);

  /**
   * Gets priority relative to current policy
   */
  virtual PRIORITY getPriority() const;

  /**
   * Sets priority relative to current policy
   */
  virtual void setPriority(PRIORITY priority);

 private:
  POLICY policy_;
  PRIORITY priority_;
  int stackSize_;
};

}

}

#endif //CONFLUO_RPC_THREAD_FACTORY_H

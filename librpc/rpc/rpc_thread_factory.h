/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef CONFLUO_RPC_THREAD_FACTORY_H
#define CONFLUO_RPC_THREAD_FACTORY_H

#include <thrift/concurrency/Thread.h>
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
  std::shared_ptr<::apache::thrift::concurrency::Thread>
  newThread(std::shared_ptr<::apache::thrift::concurrency::Runnable> runnable) const override;

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

#ifndef LOCKS_H_
#define LOCKS_H_

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <pthread.h>
#include <poll.h>

#ifdef CUSTOM_LOCKS

class MutexImpl {
 public:
  MutexImpl() {
    lk_ = PTHREAD_RWLOCK_INITIALIZER;
  }

  int ReadLock() {
    return pthread_rwlock_rdlock(&lk_);
  }

  int WriteLock() {
    return pthread_rwlock_wrlock(&lk_);
  }

  int Unlock() {
    return pthread_rwlock_unlock(&lk_);
  }
 private:
  pthread_rwlock_t lk_;
};

template<class MutexType>
class ReadLockImpl {
 public:
  ReadLockImpl(MutexType& mtx) : mtx_(mtx) {
    mtx_.ReadLock();
  }

  ~ReadLockImpl() {
    mtx_.Unlock();
  }

 private:
  MutexType& mtx_;
};

template<class MutexType>
class WriteLockImpl {
 public:
  WriteLockImpl(MutexType& mtx) : mtx_(mtx) {
    mtx_.WriteLock();
  }

  ~WriteLockImpl() {
    mtx_.Unlock();
  }

 private:
  MutexType& mtx_;
};

typedef MutexImpl Mutex;
typedef WriteLockImpl<Mutex> WriteLock;
typedef ReadLockImpl<Mutex> ReadLock;

#endif

#ifdef BOOST_LOCKS
typedef boost::shared_mutex Mutex;
typedef boost::unique_lock<Mutex> WriteLock;
typedef boost::shared_lock<Mutex> ReadLock;
#endif

#ifdef STL_LOCKS
typedef std::mutex Mutex;
typedef std::lock_guard<Mutex> WriteLock;
typedef std::lock_guard<Mutex> ReadLock;
#endif

#endif /* LOCKS_H_ */

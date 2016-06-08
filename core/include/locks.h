#ifndef LOCKS_H_
#define LOCKS_H_

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

typedef boost::shared_mutex Mutex;
typedef boost::unique_lock<Mutex> WriteLock;
typedef boost::shared_lock<Mutex> ReadLock;

#endif /* LOCKS_H_ */

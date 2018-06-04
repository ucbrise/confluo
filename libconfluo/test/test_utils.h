#ifndef CONFLUO_TEST_TEST_UTILS_H_
#define CONFLUO_TEST_TEST_UTILS_H_

#include "gtest/gtest.h"

namespace confluo {
namespace test {
class test_utils {
 public:
  template<typename F>
  static inline bool test_fail(F f) {
    bool fail = false;
    try {
      f();
    } catch (std::exception &ex) {
      fail = true;
    }
    return fail;
  }
};

}
}

#endif /* CONFLUO_TEST_TEST_UTILS_H_ */

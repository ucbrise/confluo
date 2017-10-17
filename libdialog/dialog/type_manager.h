#ifndef DIALOG_TYPE_MANAGER_H_
#define DIALOG_TYPE_MANAGER_H_

#include "relational_ops.h"
#include "arithmetic_ops.h"
#include "key_ops.h"
#include "data_types.h"
#include "exceptions.h"
#include "atomic.h"
#include "ip_address.h"

namespace dialog {

class type_manager {
 public:
  /**
   * Registers a type to the manager
   */
  static void register_type(std::vector<rel_ops_t> rel_ops,
          std::vector<unary_ops_t> un_ops, std::vector<binary_ops_t>
          binary_ops, std::vector<key_ops_t> key_ops) {
      id = atomic::faa(id, 1);
      // TODO Pass in the class type, so can do sizeof(class type)
      data_types.push_back(data_type(id));

      // TODO Add in fields for min, max, one, zero

      RELOPS.push_back(rel_ops);
      UNOPS.push_back(un_ops);
      BINOPS.push_back(binary_ops);
      KEYOPS.push_back(key_ops);
  }

  static void register_primitives() {
      MIN = init_min();
      MAX = init_max();
      ONE = init_one();
      ZERO = init_zero();

      RELOPS = init_rops();
      UNOPS = init_uops();
      BINOPS = init_bops();
      KEYOPS = init_kops();

      id = atomic::faa(id, 0);

      // TODO Probably should use compare and swap instead of <
      for (; id < 9; id = atomic::faa(id, 1)) {
          data_types.push_back(data_type(id));
      }
  }

  /**
   * Deregisters the type
   */
  static void deregister_type() {

  }

  typedef void (*regex_fn)(void* res, std::string& parse);

  static std::vector<void*> MIN;
  static std::vector<void*> MAX;
  static std::vector<void*> ONE;
  static std::vector<void*> ZERO;

  static std::vector<rel_ops_t> RELOPS;
  static std::vector<unary_ops_t> UNOPS;
  static std::vector<binary_ops_t> BINOPS;
  static std::vector<key_op> KEYOPS;

  static std::vector<regex_fn> REGEXES;

  static std::vector<data_type> data_types;

 private:
  int id;
};


}

#endif /* DIALOG_THREAD_MANAGER_H_ */

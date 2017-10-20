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

  static std::vector<data_type> data_types;
  static std::atomic<std::uint16_t> id;

  /**
   * Registers a type to the manager
   */
  static void register_type(size_t size, rel_ops_t rel_ops,
          unary_ops_t un_ops, binary_ops_t binary_ops, 
          key_op key_ops, void* min, void* max, void* one, void* zero) {
      id = atomic::faa(&id, (uint16_t) 1);
      data_types.push_back(data_type(id, size));

      MIN.push_back(min);
      MAX.push_back(max);
      ONE.push_back(one);
      ZERO.push_back(zero);

      RELOPS.push_back(rel_ops);
      UNOPS.push_back(un_ops);
      BINOPS.push_back(binary_ops);
      KEYOPS.push_back(key_ops);
  }

  static void register_primitives() {
      id = atomic::faa(&id, (uint16_t) 8);
      data_types.push_back(NONE_TYPE);
      data_types.push_back(BOOL_TYPE);
      data_types.push_back(CHAR_TYPE);

      data_types.push_back(SHORT_TYPE);
      data_types.push_back(INT_TYPE);
      data_types.push_back(LONG_TYPE);

      data_types.push_back(FLOAT_TYPE);
      data_types.push_back(DOUBLE_TYPE);
      data_types.push_back(STRING_TYPE(0));
  }

  /**
   * Deregisters the type
   */
  static void deregister_type() {

  }

 private:
  //static std::atomic<std::uint16_t> id;
};


}

#endif /* DIALOG_THREAD_MANAGER_H_ */

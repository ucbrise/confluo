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

struct type_definition {
    size_t size;
    rel_ops_t rel_ops;
    unary_ops_t un_ops;

    binary_ops_t binary_ops;
    key_op key_ops;

    void* min;
    void* max;

    void* one;
    void* zero;

    type_definition(size_t _size, rel_ops_t _rel_ops,
          unary_ops_t _un_ops, binary_ops_t _binary_ops, 
          key_op _key_ops, void* _min, void* _max, void* _one, 
          void* _zero) : size(_size),
                         rel_ops(_rel_ops),
                         un_ops(_un_ops),
                         binary_ops(_binary_ops),
                         key_ops(_key_ops),
                         min(_min),
                         max(_max),
                         one(_one),
                         zero(_zero) {
    }
};

class type_manager {
 public:

  static std::vector<data_type> data_types;
  static std::atomic<std::uint16_t> id;

  /**
   * Registers a type to the manager
   */
  static void register_type(type_definition type_def) {
      id = atomic::faa(&id, (uint16_t) 1);
      data_types.push_back(data_type(id, type_def.size));

      MIN.push_back(type_def.min);
      MAX.push_back(type_def.max);
      ONE.push_back(type_def.one);
      ZERO.push_back(type_def.zero);

      RELOPS.push_back(type_def.rel_ops);
      UNOPS.push_back(type_def.un_ops);
      BINOPS.push_back(type_def.binary_ops);
      KEYOPS.push_back(type_def.key_ops);
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
};


}

#endif /* DIALOG_THREAD_MANAGER_H_ */

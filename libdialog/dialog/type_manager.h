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

    std::string (*name)();

    type_definition(size_t _size, rel_ops_t _rel_ops,
          unary_ops_t _un_ops, binary_ops_t _binary_ops, 
          key_op _key_ops, void* _min, void* _max, void* _one, 
          void* _zero, std::string (*_name)()) : size(_size),
                         rel_ops(_rel_ops),
                         un_ops(_un_ops),
                         binary_ops(_binary_ops),
                         key_ops(_key_ops),
                         min(_min),
                         max(_max),
                         one(_one),
                         zero(_zero),
                         name(_name) {
    }
};

class type_manager {
 public:
  static std::vector<struct data_type> data_types;

  /**
   * Registers a type to the manager
   */
  static uint16_t register_type(type_definition type_def) {
      atomic::faa(&id, (uint16_t) 1);
      data_types.push_back(data_type(id, type_def.size));

      MIN.push_back(type_def.min);
      MAX.push_back(type_def.max);
      ONE.push_back(type_def.one);
      ZERO.push_back(type_def.zero);

      RELOPS.push_back(type_def.rel_ops);
      UNOPS.push_back(type_def.un_ops);
      BINOPS.push_back(type_def.binary_ops);
      KEYOPS.push_back(type_def.key_ops);
      TO_STRINGS.push_back(type_def.name);

      return id;
  }

  static uint16_t register_primitives() {
      data_types.push_back(data_type(0, 0));
      data_types.push_back(data_type(1, sizeof(bool)));
      data_types.push_back(data_type(2, sizeof(int8_t)));

      data_types.push_back(data_type(3, sizeof(int16_t)));
      data_types.push_back(data_type(4, sizeof(int32_t)));
      data_types.push_back(data_type(5, sizeof(int64_t)));

      data_types.push_back(data_type(6, sizeof(float)));
      data_types.push_back(data_type(7, sizeof(double)));

      size_t size = 10000;
      data_types.push_back(data_type(8, size));

      atomic::faa(&id, (uint16_t) 8);
      return id;
  }

  /**
   * Deregisters the type
   */
  static void deregister_type() {

  }

 private:
   static std::atomic<uint16_t> id;
};

//register_primitives();

}

#endif /* DIALOG_THREAD_MANAGER_H_ */

#ifndef DIALOG_TYPE_PROPERTIES_H_
#define DIALOG_TYPE_PROPERTIES_H_

#include "arithmetic_ops.h"
#include "relational_ops.h"
#include "key_ops.h"
#include "data_types.h"

namespace dialog {

struct type_properties {
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
  data (*parse)(const std::string&);

  void (*serialize)(std::ostream&, data&);
  void (*deserialize)(std::istream&, data&);

  type_properties(size_t _size, rel_ops_t _rel_ops, unary_ops_t _un_ops,
                  binary_ops_t _binary_ops, key_op _key_ops, void* _min,
                  void* _max, void* _one, void* _zero, std::string (*_name)(),
                  data (*_parse)(const std::string&),
                  void (*_serialize)(std::ostream&, data&),
                  void (*_deserialize)(std::istream&, data&))
      : size(_size),
        rel_ops(_rel_ops),
        un_ops(_un_ops),
        binary_ops(_binary_ops),
        key_ops(_key_ops),
        min(_min),
        max(_max),
        one(_one),
        zero(_zero),
        name(_name),
        parse(_parse),
        serialize(_serialize),
        deserialize(_deserialize) {
  }
};

}

#endif /* DIALOG_TYPE_PROPERTIES_H_ */

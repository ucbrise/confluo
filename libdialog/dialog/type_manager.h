#ifndef DIALOG_TYPE_MANAGER_H_
#define DIALOG_TYPE_MANAGER_H_

#include "relational_ops.h"
#include "arithmetic_ops.h"
#include "key_ops.h"
#include "data_types.h"
#include "exceptions.h"
#include "atomic.h"

namespace dialog {

struct type_operators {
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

    type_operators(size_t _size, rel_ops_t _rel_ops,
          unary_ops_t _un_ops, binary_ops_t _binary_ops, 
          key_op _key_ops, void* _min, void* _max, void* _one, 
          void* _zero, std::string (*_name)(), data (*_parse)(
              const std::string&)) : 
                         size(_size),
                         rel_ops(_rel_ops),
                         un_ops(_un_ops),
                         binary_ops(_binary_ops),
                         key_ops(_key_ops),
                         min(_min),
                         max(_max),
                         one(_one),
                         zero(_zero),
                         name(_name),
                         parse(_parse) {
    }
};

static std::vector<data_type> data_types;
static std::atomic<uint16_t> id;

class type_manager {
 public:
  //static std::vector<data_type> data_types;

  /**
   * Registers a type to the manager
   */
  static uint16_t register_type(type_operators type_def) {
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

      PARSERS.push_back(type_def.parse);

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

      // Insert an arbitrary string data_type not actually used
      // Use STRING_TYPE(size) for the actual string
      // Can't push to data_types vector because don't know size in
      // advance
      // Done to keep the indexing nice
      data_types.push_back(data_type(8, 10000));

      atomic::faa(&id, (uint16_t) 8);
      return id;
  }

  static uint16_t get_id_from_type_name(std::string type_name) {
      for (unsigned int i = 0; i < data_types.size(); i++) {
          if (type_name.compare(data_types[i].to_string()) == 0) {
              return i;
          }
      }
      return -1;
  }

  static bool is_valid_id(uint16_t other_id) {
      return other_id >= 0 && other_id <= id;
  }

  static bool is_primitive(uint16_t other_id) {
      return other_id >= 0 && other_id <= 8;
  }

  /**
   * Deregisters the type
   */
  static void deregister_type() {

  }

 //private:
 //  static std::atomic<uint16_t> id;
};

static uint16_t tid = type_manager::register_primitives();

static data_type NONE_TYPE = data_types[
    type_manager::get_id_from_type_name(TO_STRINGS[0]())];
static data_type BOOL_TYPE = data_types[
    type_manager::get_id_from_type_name(TO_STRINGS[1]())];
static data_type CHAR_TYPE = data_types[
    type_manager::get_id_from_type_name(TO_STRINGS[2]())];
static data_type SHORT_TYPE = data_types[
    type_manager::get_id_from_type_name(TO_STRINGS[3]())];
static data_type INT_TYPE = data_types[
    type_manager::get_id_from_type_name(TO_STRINGS[4]())];
static data_type LONG_TYPE = data_types[
    type_manager::get_id_from_type_name(TO_STRINGS[5]())];
static data_type FLOAT_TYPE = data_types[
    type_manager::get_id_from_type_name(TO_STRINGS[6]())];
static data_type DOUBLE_TYPE = data_types[
    type_manager::get_id_from_type_name(TO_STRINGS[7]())];
static data_type STRING_TYPE(size_t size) {
  return data_type(type_id::D_STRING, size);
}

}

#endif /* DIALOG_THREAD_MANAGER_H_ */

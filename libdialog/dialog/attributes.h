#ifndef DIALOG_ATTRIBUTES_H_
#define DIALOG_ATTRIBUTES_H_

namespace dialog {

struct attribute_id_t {
  uint32_t idx :32;
  uint32_t length :32;
};

struct attribute {
  attribute_id_t id;
  void* value;

  template<typename T>
  T get() {
    return *((T*) value);
  }
};

typedef std::vector<attribute> attribute_list;

}

#endif /* DIALOG_ATTRIBUTES_H_ */

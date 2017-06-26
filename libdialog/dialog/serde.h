#ifndef DIALOG_SERDE_H_
#define DIALOG_SERDE_H_

namespace dialog {

// TODO: Add documentation

template<typename T>
struct serializer {
  static size_t size(const T& o) {
    return sizeof(T);
  }

  static void serialize(void* dst, const T& o) {
    memcpy(dst, &o, sizeof(T));
  }
};

template<>
struct serializer<std::string> {
  static size_t size(const std::string& o) {
    return o.length();
  }

  static void serialize(void* dst, const std::string& o) {
    memcpy(dst, o.c_str(), o.length());
  }
};

template<typename T>
struct deserializer {
  static void deserialize(const void* src, T* o) {
    memcpy(o, src, sizeof(T));
  }
};

}

#endif /* DIALOG_SERDE_H_ */

#ifndef GRAPHSTORE_SERDE_H_
#define GRAPHSTORE_SERDE_H_

#include "link.h"
#include "node.h"

template<>
struct datastore::serializer<graphstore::node_op> {
  static size_t size(const graphstore::node_op& o) {
    return sizeof(int32_t) + sizeof(uint16_t) + o.data.length() * sizeof(char);
  }

  void serialize(uint8_t* dst, const struct graphstore::node_op& o) {
    memcpy(dst, &o.type, sizeof(int32_t));
    uint16_t length = o.data.length();
    memcpy(dst + sizeof(int32_t), &length, sizeof(uint16_t));
    memcpy(dst + sizeof(int32_t) + sizeof(uint16_t), o.data.c_str(),
           length * sizeof(char));
  }
};

template<>
struct datastore::deserializer<graphstore::node_op> {
  static void deserialize(const uint8_t* src, graphstore::node_op* o) {
    memcpy(&o->type, src, sizeof(int32_t));
    uint16_t length;
    memcpy(&length, src + sizeof(int32_t), sizeof(uint16_t));
    const char* buf = src + sizeof(int32_t) + sizeof(uint16_t);
    o->data.assign(buf, length);
  }
};

template<>
struct datastore::serializer<graphstore::link_op> {
  static size_t size(const graphstore::link_op& o) {
    return sizeof(int64_t) * 4 + sizeof(uint16_t)
        + o.data.length() * sizeof(char);
  }

  static void serialize(uint8_t* dst, const graphstore::link_op& o) {
    memcpy(dst, &o.id1, sizeof(int64_t));
    memcpy(dst + sizeof(int64_t), &o.link_type, sizeof(int64_t));
    memcpy(dst + 2 * sizeof(int64_t), &o.id2, sizeof(int64_t));
    uint16_t length = o.data.length();
    memcpy(dst + 3 * sizeof(int64_t), &length, sizeof(uint16_t));
    memcpy(dst + 3 * sizeof(int64_t) + sizeof(uint16_t), o.data.c_str(),
           length * sizeof(char));
  }
};

template<>
struct datastore::deserializer<graphstore::link_op> {
  static void deserialize(const uint8_t* src, graphstore::link_op* o) {
    memcpy(&o->id1, src, sizeof(int64_t));
    memcpy(&o->link_type, src + sizeof(int64_t), sizeof(int64_t));
    memcpy(&o->id2, src + 2 * sizeof(int64_t), sizeof(int64_t));
    uint16_t length;
    memcpy(&length, src + 3 * sizeof(int64_t), sizeof(uint16_t));
    const char* buf = src + 3 * sizeof(int64_t) + sizeof(uint16_t);
    o->data.assign(buf, length);
  }
};

#endif /* GRAPHSTORE_SERDE_H_ */

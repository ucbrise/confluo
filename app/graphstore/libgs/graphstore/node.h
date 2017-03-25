#ifndef GRAPHSTORE_NODE_H_
#define GRAPHSTORE_NODE_H_

#include "object.h"

#define INVALID_NODE UINT64_MAX
#define VALID_NODE   UINT64_MAX - 1

namespace graphstore {

struct node_op {
  uint64_t id;
  int32_t type;
  std::string data;
  bool delete_op;

  node_op() {
    id = UINT64_MAX;
    type = -1;
    delete_op = false;
  }

  static node_op empty() {
    node_op op;
    return op;
  }
};

typedef monolog::monolog_relaxed<uint64_t, 24> adj_list;

struct node : public datastore::stateful {
 public:
  /** Version of node **/
  uint64_t version;

  /** Type of node **/
  int32_t type;

  /** Data associated with node **/
  std::string data;

  /** Node neighbors **/
  adj_list* neighbors;

  node()
      : stateful() {
    version = UINT64_MAX;
    type = -1;
    neighbors = nullptr;
  }

  node(const node& rhs)
      : stateful(rhs) {
    *this = rhs;
  }

  node(const node_op& rhs)
      : node() {
    if (!rhs.delete_op) {
      type = rhs.type;
      data = rhs.data;
      neighbors = new adj_list;
    }
  }

  node& operator=(const node& rhs) {
    version = rhs.version;
    type = rhs.type;
    data = rhs.data;
    neighbors = rhs.neighbors;
    return *this;
  }

  node& operator=(const node_op& rhs) {
    if (!rhs.delete_op) {
      type = rhs.type;
      data = rhs.data;
      neighbors = new adj_list;
    }
    return *this;
  }

  node_op clone(int64_t id) const {
    if (type == -1)
      return node_op::empty();

    node_op n;
    n.id = id;
    n.type = type;
    n.data = data;
    return n;
  }
};

}

#endif /* GRAPHSTORE_NODE_H_ */

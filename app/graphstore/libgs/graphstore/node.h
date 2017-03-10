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
};

typedef monolog::monolog_relaxed<uint64_t, 24> adj_list;

struct node: public datastore::object {
 public:
  /** Type of node **/
  int32_t type;

  /** Data associated with node **/
  std::string data;

  /** Node neighbors **/
  adj_list* neighbors;

  node() {
    type = -1;
    neighbors = nullptr;
  }

  node& operator=(const node_op& rhs) {
    type = rhs.type;
    data = rhs.data;
    return *this;
  }

  node_op clone(int64_t id) const {
    node_op n;
    n.id = id;
    n.type = type;
    n.data = data;
    return n;
  }
};

}

#endif /* GRAPHSTORE_NODE_H_ */

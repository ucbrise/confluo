#ifndef GRAPHSTORE_LINK_H_
#define GRAPHSTORE_LINK_H_

#include <cstdint>

namespace graphstore {

#define INVALID_LINK  UINT64_MAX
#define VALID_LINK    UINT64_MAX - 1

struct link_op {
  uint64_t version;
  int64_t id1;
  int64_t id2;
  int64_t link_type;
  int64_t time;
  std::string data;
  bool delete_op;

  link_op() {
    version = UINT64_MAX;
    id1 = -1;
    id2 = -1;
    link_type = -1;
    time = -1;
    delete_op = false;
  }

  bool operator<(const link_op& rhs) const {
    return (time == rhs.time) ? id2 < rhs.id2 : time > rhs.time;
  }
};

struct link : public datastore::object {
 public:
  /** The node id of the source of directed edge **/
  int64_t id1;

  /** The node id of the target of directed edge **/
  int64_t id2;

  /** Type of link **/
  int64_t link_type;

  /** time is the sort key for links.  Often it contains a timestamp,
   but it can be used as a arbitrary user-defined sort key. **/
  int64_t time;

  /** Arbitrary payload data **/
  std::string data;

  /**
   * Default constructor
   */
  link() {
    id1 = -1;
    id2 = -1;
    link_type = -1;
    time = -1;
  }

  link& operator=(const link_op& rhs) {
    id1 = rhs.id1;
    id2 = rhs.id2;
    link_type = rhs.link_type;
    time = rhs.time;
    data = rhs.data;
    return *this;
  }

  link_op clone(uint64_t version) {
    link_op l;
    l.version = version;
    l.id1 = id1;
    l.id2 = id2;
    l.link_type = link_type;
    l.time = time;
    l.data = data;
    return l;
  }
};

}

#endif /* GRAPHSTORE_LINK_H_ */

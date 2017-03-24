#ifndef GRAPHSTORE_GRAPH_STORE_H_
#define GRAPHSTORE_GRAPH_STORE_H_

#include <vector>
#include <set>
#include <map>

#include "concurrency_control.h"
#include "monolog.h"
#include "node.h"
#include "link.h"

namespace graphstore {

template<typename graph_tail>
class graph_store {
 public:
  typedef monolog::monolog_relaxed<node, 32> node_log;
  typedef monolog::monolog_relaxed<link, 32> link_log;

  graph_store();

  /* Node operations */
  uint64_t add_node(const node_op& n);
  node_op get_node(int64_t type, uint64_t id) const;
  bool update_node(const node_op& n);
  bool delete_node(int64_t type, uint64_t id);

  /* Link operations */
  bool add_link(const link_op& a);
  bool delete_link(int64_t id1, int64_t link_type, int64_t id2);
  bool update_link(const link_op& a);
  link_op get_link(int64_t id1, int64_t link_type, int64_t id2) const;
  std::vector<link_op> multiget_links(int64_t id1, int64_t link_type,
                                      std::set<int64_t> id2s) const;
  std::set<link_op> get_link_list(int64_t id1, int64_t link_type) const;
  std::set<link_op> get_link_list(int64_t id1, int64_t link_type,
                                  int64_t min_ts, int64_t max_ts, int64_t off,
                                  int64_t limit) const;
  size_t count_links(int64_t id1, int64_t link_type) const;

 private:
  void follow_update_refs(uint64_t& id, uint64_t tail) const;
  bool is_valid(uint64_t id, uint64_t tail) const;

  template<typename F>
  void filter_link_ids(std::map<int64_t, link_op>& links, adj_list* list,
                       int64_t link_type, uint64_t tail, F&& f) const;

  node_log *ndata_;
  link_log *ldata_;
  graph_tail tail_;
};

}

#endif /* GRAPHSTORE_NODE_TABLE_H_ */

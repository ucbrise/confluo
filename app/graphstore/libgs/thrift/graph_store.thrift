namespace java edu.berkeley.cs.graphstore
namespace cpp graphstore
namespace py graphstore

struct TNode {
  1: i64 id,
  2: i64 type,
  3: string data,
}

struct TLink {
  1: i64 version,
  2: i64 id1,
  3: i64 link_type,
  4: i64 id2,
  5: i64 time,
  6: string data,
}

service GraphStoreService {
  // Management
  void init_connection(),
  void destroy_connection(),

  // Node API
  i64 add_node(1: TNode n),
  TNode get_node(1: i64 type, 2: i64 id),
  bool update_node(1: TNode n),
  bool delete_node(1: i64 type, 2: i64 id),

  // Link API
  bool add_link(1: TLink a),
  bool update_link(1: TLink a),
  bool delete_link(1: i64 id1, 2: i64 link_type, 3: i64 id2),
  TLink get_link(1: i64 id1, 2: i64 link_type, 3: i64 id2),
  list<TLink> multiget_link(1: i64 id1, 2: i64 link_type, 3: set<i64> id2s),
  list<TLink> get_link_list(1: i64 id1, 2: i64 link_type),
  list<TLink> get_link_list_range(1: i64 id1, 2: i64 link_type, 3: i64 min_ts, 
                                  4: i64 max_ts, 5: i64 off, 6: i64 limit),
  i64 count_links(1: i64 id1, 2: i64 link_type),
  
  // Snapshot
  i64 begin_snapshot(),
  bool end_snapshot(1: i64 tail),

  // Traverse
  list<TLink> traverse(1: i64 id, 2: i64 link_type, 3: i64 depth, 4: list<i64> snapshot, 5: set<i64> visited)
}

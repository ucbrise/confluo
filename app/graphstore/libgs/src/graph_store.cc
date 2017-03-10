#include "graph_store.h"

using namespace graphstore;

graph_store::graph_store() {
  write_tail_.store(0ULL, std::memory_order_relaxed);
  read_tail_.store(0ULL, std::memory_order_relaxed);
  ndata_ = new node_log;
  ldata_ = new link_log;
}

// Helper functions
graph_tail graph_store::start_node_append_op() {
  return graph_tail_ops::increment_node_tail(write_tail_);
}

void graph_store::end_node_append_op(graph_tail tail) {
  return graph_tail_ops::update_tail(read_tail_, tail,
                                     tail + tail.node_tail.one());
}

graph_tail graph_store::start_link_append_op() {
  return graph_tail_ops::increment_link_tail(write_tail_);
}

void graph_store::end_link_append_op(graph_tail tail) {
  return graph_tail_ops::update_tail(read_tail_, tail,
                                     tail + tail.link_tail.one());
}

graph_tail graph_store::get_graph_tail() const {
  return read_tail_.load(std::memory_order_release);
}

bool graph_store::follow_node_update_ptrs(uint64_t& id, graph_tail tail) const {
  uint64_t id_next;
  while ((id_next = (*ndata_)[id].state.get()) < tail.node_tail)
    id = id_next;

  if (id >= tail.node_tail)
    return false;

  return true;
}

template<typename F>
void graph_store::filter_link_ids(std::map<int64_t, link_op>& links,
                                  adj_list* list, int64_t link_type,
                                  graph_tail tail, F&& f) const {
  size_t size = list->size();
  for (size_t i = 0; i < size; i++) {
    uint64_t link_id = list->at(i);
    link& l = (*ldata_)[link_id];
    if (l.link_type == link_type && f(l)) {
      int64_t id2 = l.id2;
      if (links.find(id2) == links.end())
        links.insert(std::make_pair(id2, l.clone(link_id)));
      else if (link_id > links[id2].version)
        links[id2] = l.clone(link_id);
    }
  }
}

// API implementations
uint64_t graph_store::add_node(const node_op& n) {
  graph_tail t = start_node_append_op();
  (*ndata_)[t.node_tail] = n;
  (*ndata_)[t.node_tail].neighbors = new adj_list;
  (*ndata_)[t.node_tail].state.initalize();
  end_node_append_op(t);
  return t.node_tail;
}

node_op graph_store::get_node(int64_t type, uint64_t id) const {
  node_op n;
  graph_tail t = get_graph_tail();

  // Node does not exist yet
  if (id > t.node_tail)
    return n;

  uint64_t original_id = id;
  follow_node_update_ptrs(id, t);
  node& internal_node = (*ndata_)[id];

  if (internal_node.type != type)
    return n;

  n = internal_node.clone(original_id);
  return n;
}

bool graph_store::update_node(const node_op& n) {
  uint64_t id = n.id;
  graph_tail t = get_graph_tail();

  // Node does not exist yet
  if (id > t.node_tail)
    return false;

  // Try again, since the update chain ends in a node with id > read_tail
  if (!follow_node_update_ptrs(id, t)) {
    fprintf(stderr, "Chain ends in a node beyond read tail\n");
    return update_node(n);
  }

  node& old_node = (*ndata_)[id];

  // The node does not match the type
  if (old_node.type != n.type)
    return false;

  // Express intent to update
  bool success = old_node.state.mark_updating();
  if (!success) {
    fprintf(stderr, "Could not mark node as updating\n");
    return update_node(n);
  }

  t = start_node_append_op();
  uint64_t new_id = t.node_tail;

  node& new_node = (*ndata_)[new_id];
  if (!n.delete_op) {
    new_node = n;
    new_node.neighbors = old_node.neighbors;
  }

  old_node.state.update(new_id);
  end_node_append_op(t);

  return true;
}

bool graph_store::delete_node(int64_t type, uint64_t id) {
  node_op n;
  n.id = id;
  n.type = type;
  n.delete_op = true;
  return update_node(n);
}

bool graph_store::add_link(const link_op& a) {
  graph_tail t = get_graph_tail();
  if (static_cast<uint64_t>(a.id1) > t.node_tail)
    return false;

  t = start_link_append_op();
  link& l = (*ldata_)[t.link_tail];
  l = a;
  if (a.delete_op)
    l.id1 = -1;
  l.state.initalize();
  (*ndata_)[a.id1].neighbors->push_back(t.link_tail);
  end_link_append_op(t);
  return true;
}

bool graph_store::update_link(const link_op& a) {
  return add_link(a);
}

bool graph_store::delete_link(int64_t id1, int64_t link_type, int64_t id2) {
  link_op dl;
  dl.id1 = id1;
  dl.id2 = id2;
  dl.link_type = link_type;
  dl.delete_op = true;
  return add_link(dl);
}

link_op graph_store::get_link(int64_t id1, int64_t link_type,
                              int64_t id2) const {
  graph_tail t = get_graph_tail();
  if (static_cast<uint64_t>(id1) > t.node_tail) {
    link_op l;
    return l;
  }

  adj_list* list = (*ndata_)[id1].neighbors;
  std::map<int64_t, link_op> links;
  filter_link_ids(links, list, link_type, t, [id2](link& l) -> bool {
    return l.id2 == id2;
  });

  if (links.empty() || links.at(id2).id1 == -1) {
    link_op l;
    return l;
  }

  return links.at(id2);
}

std::vector<link_op> graph_store::multiget_links(int64_t id1, int64_t link_type,
                                                 std::set<int64_t> id2s) const {
  graph_tail t = get_graph_tail();
  std::vector<link_op> l;
  if (static_cast<uint64_t>(id1) > t.node_tail) {
    std::vector<link_op> l;
    return l;
  }

  adj_list* list = (*ndata_)[id1].neighbors;
  std::map<int64_t, link_op> links;
  filter_link_ids(links, list, link_type, t, [&id2s](link& l) -> bool {
    return id2s.find(l.id2) != id2s.end();
  });

  for (const auto& entry : links)
    if (entry.second.id1 != -1)
      l.push_back(entry.second);

  return l;
}

std::set<link_op> graph_store::get_link_list(int64_t id1,
                                             int64_t link_type) const {
  graph_tail t = get_graph_tail();
  std::set<link_op> l;
  if (static_cast<uint64_t>(id1) > t.node_tail)
    return l;

  adj_list* list = (*ndata_)[id1].neighbors;
  std::map<int64_t, link_op> links;
  filter_link_ids(links, list, link_type, t, [](link& l) -> bool {
    return true;
  });

  for (const auto& entry : links)
    if (entry.second.id1 != -1)
      l.insert(entry.second);

  return l;
}

std::set<link_op> graph_store::get_link_list(int64_t id1, int64_t link_type,
                                             int64_t min_ts, int64_t max_ts,
                                             int64_t off, int64_t limit) const {
  graph_tail t = get_graph_tail();
  std::set<link_op> l;
  if (static_cast<uint64_t>(id1) > t.node_tail)
    return l;

  adj_list* list = (*ndata_)[id1].neighbors;
  std::map<int64_t, link_op> links;
  filter_link_ids(links, list, link_type, t, [min_ts, max_ts](link& l) -> bool {
    return l.time >= min_ts && l.time <= max_ts;
  });

  for (const auto& entry : links)
    if (entry.second.id1 != -1)
      l.insert(entry.second);

  auto it = l.begin();
  std::advance(it, off);
  std::set<link_op> output;
  for (; it != l.end() && output.size() < static_cast<size_t>(limit); it++)
    output.insert(*it);
  return output;
}

size_t graph_store::count_links(int64_t id1, int64_t link_type) const {
  graph_tail t = get_graph_tail();
  size_t count = 0;
  if (static_cast<uint64_t>(id1) > t.node_tail)
    return count;

  adj_list* list = (*ndata_)[id1].neighbors;
  std::map<int64_t, link_op> links;
  filter_link_ids(links, list, link_type, t, [](link& l) -> bool {
    return true;
  });

  for (const auto& entry : links)
    if (entry.second.id1 != -1)
      count++;

  return count;
}

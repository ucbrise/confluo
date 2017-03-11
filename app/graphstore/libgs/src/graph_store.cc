#include "graph_store.h"

using namespace graphstore;

graph_store::graph_store() {
  write_tail_.store(0ULL, std::memory_order_relaxed);
  read_tail_.store(0ULL, std::memory_order_relaxed);
  ndata_ = new node_log;
  ldata_ = new link_log;
}

// Helper functions
uint64_t graph_store::start_write_op() {
  return write_tail_.fetch_add(1ULL, std::memory_order_release);
}

void graph_store::end_write_op(uint64_t tail) {
  uint64_t old_tail;
  do {
    old_tail = tail;
  } while (!read_tail_.compare_exchange_weak(old_tail, tail + 1,
                                             std::memory_order_acquire,
                                             std::memory_order_release));
}

uint64_t graph_store::get_graph_tail() const {
  return read_tail_.load(std::memory_order_release);
}

void graph_store::follow_update_refs(uint64_t& id, uint64_t tail) const {
  while (true) {
    uint64_t state;
    do {
      state = (*ndata_)[id].state.get();
    } while (state == datastore::object_state::uninitialized
        || state == datastore::object_state::updating);

    if (state == datastore::object_state::initialized
        || (*ndata_)[state].version >= tail) {
      break;
    }

    // Updated version exists within tail
    id = state;
  }
}

template<typename F>
void graph_store::filter_link_ids(std::map<int64_t, link_op>& links,
                                  adj_list* list, int64_t link_type,
                                  uint64_t tail, F&& f) const {
  size_t size = list->size();
  for (size_t i = 0; i < size; i++) {
    link& l = (*ldata_)[list->at(i)];
    if (l.link_type == link_type && f(l)) {
      int64_t id2 = l.id2;
      if (links.find(id2) == links.end())
        links.insert(std::make_pair(id2, l.clone()));
      else if (l.version > links[id2].version)
        links[id2] = l.clone();
    }
  }
}

// API implementations
uint64_t graph_store::add_node(const node_op& n) {
  uint64_t t = start_write_op();
  node internal_node = n;
  uint64_t id = ndata_->push_back(internal_node);
  (*ndata_)[id].version = t;
  (*ndata_)[id].state.initalize();
  end_write_op(t);
  return id;
}

node_op graph_store::get_node(int64_t type, uint64_t id) const {
  uint64_t t = get_graph_tail();

  // Node with specified type does not exist
  if (id >= ndata_->size() || (*ndata_)[id].version >= t
      || (*ndata_)[id].type != type)
    return node_op::empty();

  uint64_t original_id = id;
  follow_update_refs(id, t);
  return (*ndata_)[id].clone(original_id);
}

bool graph_store::update_node(const node_op& n) {
  uint64_t id = n.id;
  uint64_t t = get_graph_tail();

  // Node with specified type does not exist
  if (id >= ndata_->size() || (*ndata_)[id].version >= t
      || (*ndata_)[id].type != n.type)
    return false;

  follow_update_refs(id, t);
  node& old_node = (*ndata_)[id];

  // Try to mark node as 'updating'; retry on failure
  if (!old_node.state.mark_updating())
    return update_node(n);

  // Only one op will succeeding in marking the node as 'updating'
  // Add the updated node as a new entry and link it to its previous version
  uint64_t new_id = add_node(n);
  old_node.state.update(new_id);
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
  if (static_cast<uint64_t>(a.id1) >= ndata_->size()) {
    fprintf(stderr, "Could not add link: %lld, %zu\n", a.id1, ndata_->size());
    return false;
  }

  uint64_t t = start_write_op();
  link l = a;
  uint64_t link_id = ldata_->push_back(l);
  (*ldata_)[link_id].version = t;
  (*ldata_)[link_id].state.initalize();
  (*ndata_)[a.id1].neighbors->push_back(link_id);
  end_write_op(t);
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
  uint64_t t = get_graph_tail();
  if (static_cast<uint64_t>(id1) >= ndata_->size())
    return link_op::empty();

  adj_list* list = (*ndata_)[id1].neighbors;
  std::map<int64_t, link_op> links;
  filter_link_ids(links, list, link_type, t, [id2](link& l) -> bool {
    return l.id2 == id2;
  });

  if (links.empty() || links.at(id2).id1 == -1)
    return link_op::empty();

  return links.at(id2);
}

std::vector<link_op> graph_store::multiget_links(int64_t id1, int64_t link_type,
                                                 std::set<int64_t> id2s) const {
  uint64_t t = get_graph_tail();
  std::vector<link_op> l;
  if (static_cast<uint64_t>(id1) >= ndata_->size())
    return l;

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
  uint64_t t = get_graph_tail();
  std::set<link_op> l;
  if (static_cast<uint64_t>(id1) >= ndata_->size())
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
  uint64_t t = get_graph_tail();
  std::set<link_op> l;
  if (static_cast<uint64_t>(id1) >= ndata_->size())
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
  uint64_t t = get_graph_tail();
  size_t count = 0;
  if (static_cast<uint64_t>(id1) >= ndata_->size())
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

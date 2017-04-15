#include "graph_store.h"

using namespace graphstore;

template<typename graph_tail>
graph_store<graph_tail>::graph_store() {
  ndata_ = new node_log;
  ldata_ = new link_log;
}

// Helper functions
template<typename graph_tail>
void graph_store<graph_tail>::follow_update_refs(uint64_t& id,
                                                 uint64_t tail) const {
  while (true) {
    uint64_t state = graph_tail::get_state((*ndata_)[id]);
    if (graph_tail::is_valid(state) || (*ndata_)[state].version >= tail)
      break;

    // Updated version exists within tail
    id = state;
  }
}

template<typename graph_tail>
template<typename F>
void graph_store<graph_tail>::filter_link_ids(std::map<int64_t, link_op>& links,
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
template<typename graph_tail>
uint64_t graph_store<graph_tail>::add_node(const node_op& n) {
  uint64_t t = tail_.start_write_op();
  node internal_node = n;
  internal_node.version = t;
  uint64_t id = ndata_->push_back(internal_node);
  tail_.init_object((*ndata_)[id], t);
  tail_.end_write_op(t);
  return id;
}

template<typename graph_tail>
node_op graph_store<graph_tail>::get_node(int64_t type, uint64_t id) const {
  uint64_t t = tail_.get_tail();

  // Node with specified type does not exist
  if (id >= ndata_->size() || (*ndata_)[id].version >= t
      || (*ndata_)[id].type != type)
    return node_op::empty();

  uint64_t original_id = id;
  follow_update_refs(id, t);
  return (*ndata_)[id].clone(original_id);
}

template<typename graph_tail>
bool graph_store<graph_tail>::update_node(const node_op& n) {
  uint64_t id = n.id;
  uint64_t t = tail_.get_tail();

  // Node with specified type does not exist
  if (id >= ndata_->size() || (*ndata_)[id].version >= t
      || (*ndata_)[id].type != n.type)
    return false;

  follow_update_refs(id, t);
  node& old_node = (*ndata_)[id];

  // Try to mark node as 'updating'; retry on failure
  if (!graph_tail::start_update_op(old_node))
    return update_node(n);

  // Only one op will succeeding in marking the node as 'updating'
  // Add the updated node as a new entry and link it to its previous version
  uint64_t new_id = add_node(n);
  graph_tail::end_update_op(old_node, new_id);
  return true;
}

template<typename graph_tail>
bool graph_store<graph_tail>::delete_node(int64_t type, uint64_t id) {
  node_op n;
  n.id = id;
  n.type = type;
  n.delete_op = true;
  return update_node(n);
}

template<typename graph_tail>
bool graph_store<graph_tail>::add_link(const link_op& a) {
  if (static_cast<uint64_t>(a.id1) >= ndata_->size())
    return false;

  uint64_t t = tail_.start_write_op();
  link l = a;
  l.version = t;
  uint64_t link_id = ldata_->push_back(l);
  (*ndata_)[a.id1].neighbors->push_back(link_id);
  tail_.init_object((*ldata_)[link_id], t);
  tail_.end_write_op(t);
  return true;
}

template<typename graph_tail>
bool graph_store<graph_tail>::update_link(const link_op& a) {
  return add_link(a);
}

template<typename graph_tail>
bool graph_store<graph_tail>::delete_link(int64_t id1, int64_t link_type,
                                          int64_t id2) {
  link_op dl;
  dl.id1 = id1;
  dl.id2 = id2;
  dl.link_type = link_type;
  dl.delete_op = true;
  return add_link(dl);
}

template<typename graph_tail>
link_op graph_store<graph_tail>::get_link(int64_t id1, int64_t link_type,
                                          int64_t id2) const {
  uint64_t t = tail_.get_tail();
  if (static_cast<uint64_t>(id1) >= ndata_->size()
      || (*ndata_)[id1].version >= t)
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

template<typename graph_tail>
std::vector<link_op> graph_store<graph_tail>::multiget_links(
    int64_t id1, int64_t link_type, std::set<int64_t> id2s) const {
  uint64_t t = tail_.get_tail();
  std::vector<link_op> l;
  if (static_cast<uint64_t>(id1) >= ndata_->size()
      || (*ndata_)[id1].version >= t)
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

template<typename graph_tail>
std::set<link_op> graph_store<graph_tail>::get_link_list(
    int64_t id1, int64_t link_type) const {
  uint64_t t = tail_.get_tail();
  std::set<link_op> l;
  if (static_cast<uint64_t>(id1) >= ndata_->size()
      || (*ndata_)[id1].version >= t)
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

template<typename graph_tail>
std::set<link_op> graph_store<graph_tail>::get_link_list(int64_t id1,
                                                         int64_t link_type,
                                                         int64_t min_ts,
                                                         int64_t max_ts,
                                                         int64_t off,
                                                         int64_t limit) const {
  uint64_t t = tail_.get_tail();
  std::set<link_op> l;
  if (static_cast<uint64_t>(id1) >= ndata_->size()
      || (*ndata_)[id1].version >= t)
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

template<typename graph_tail>
size_t graph_store<graph_tail>::count_links(int64_t id1,
                                            int64_t link_type) const {
  uint64_t t = tail_.get_tail();
  size_t count = 0;
  if (static_cast<uint64_t>(id1) >= ndata_->size()
      || (*ndata_)[id1].version >= t)
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

template<typename graph_tail>
uint64_t graph_store<graph_tail>::begin_snapshot() {
  return tail_.start_snapshot_op();
}

template<typename graph_tail>
bool graph_store<graph_tail>::end_snapshot(uint64_t id) {
  return tail_.end_snapshot_op(id);
}

template<typename graph_tail>
std::vector<link_op> graph_store<graph_tail>::get_links(
    int64_t id1, int64_t link_type, int64_t tail) const {
  std::vector<link_op> l;
  adj_list* list = (*ndata_)[id1].neighbors;
  std::map<int64_t, link_op> links;
  filter_link_ids(links, list, link_type, tail, [](link& l) -> bool {
    return true;
  });

  for (const auto& entry : links)
    if (entry.second.id1 != -1)
      l.push_back(entry.second);

  return l;
}

template class graphstore::graph_store<datastore::write_stalled>;
template class graphstore::graph_store<datastore::read_stalled>;

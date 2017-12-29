#ifndef CONFLUO_CONTAINER_RADIX_TREE_H_
#define CONFLUO_CONTAINER_RADIX_TREE_H_

#include <sstream>
#include <string>

#include "atomic.h"
#include "container/reflog.h"
#include "exceptions.h"
#include "types/byte_string.h"
#include "flatten.h"

namespace confluo {
namespace index {

template<typename reflog>
struct radix_tree_node {
  typedef radix_tree_node<reflog> node_t;
  typedef atomic::type<node_t*> child_t;
  typedef byte_string key_t;

  template<typename ... ARGS>
  radix_tree_node(uint8_t node_key, size_t node_width, size_t node_depth,
                  node_t* node_parent, bool, ARGS&& ... args)
      : key(node_key),
        depth(node_depth),
        is_leaf(true),
        parent(node_parent) {
    data = new reflog(std::forward<ARGS>(args)...);
  }

  radix_tree_node(uint8_t node_key, size_t node_width, size_t node_depth,
                  node_t* node_parent)
      : key(node_key),
        depth(node_depth),
        is_leaf(false),
        parent(node_parent) {
    data = new child_t[node_width];
    for (size_t i = 0; i < node_width; i++)
      atomic::init(&(children()[i]), static_cast<node_t*>(nullptr));
  }

  ~radix_tree_node() {
    if (is_leaf)
      delete refs();
    else
      delete[] children();
  }

  inline reflog*& refs() {
    return reinterpret_cast<reflog*&>(data);
  }

  inline reflog* const & refs() const {
    return reinterpret_cast<reflog* const &>(data);
  }

  inline child_t*& children() {
    return reinterpret_cast<child_t*&>(data);
  }

  inline child_t* const & children() const {
    return reinterpret_cast<child_t* const &>(data);
  }

  const node_t* first_child(size_t width) const {
    size_t cur_key = 0;
    const node_t* child = nullptr;
    while (cur_key < width
        && (child = atomic::load(&(children()[cur_key]))) == nullptr) {
      ++cur_key;
    }
    return child;
  }

  const node_t* last_child(size_t width) const {
    int16_t cur_key = width - 1;
    const node_t* child = nullptr;
    while (cur_key >= 0
        && (child = atomic::load(&(children()[cur_key]))) == nullptr) {
      --cur_key;
    }
    return child;
  }

  const node_t* next_child(uint8_t key, size_t width) const {
    size_t cur_key = key + 1;
    const node_t* child = nullptr;
    while (cur_key < width
        && (child = atomic::load(&(children()[cur_key]))) == nullptr) {
      ++cur_key;
    }
    return child;
  }

  const node_t* prev_child(uint8_t key, size_t width) const {
    if (key == 0)
      return nullptr;

    int16_t cur_key = key - 1;
    const node_t* child = nullptr;
    while (cur_key >= 0
        && (child = atomic::load(&(children()[cur_key]))) == nullptr) {
      --cur_key;
    }
    return child;
  }

  const node_t* advance(key_t& t_key, size_t t_width, size_t t_depth) const {
    if (parent == nullptr)
      return nullptr;

    const node_t* child = parent->next_child(key, t_width);
    if (child == nullptr) {
      return parent->advance(t_key, t_width, t_depth);
    } else {
      t_key[depth - 1] = child->key;
      return child->advance_descend(t_key, t_width, t_depth);
    }
  }

  const node_t* advance_descend(key_t& t_key, size_t t_width,
                                size_t t_depth) const {
    if (is_leaf)
      return this;

    const node_t* child = first_child(t_width);
    if (child == nullptr) {
      return advance(t_key, t_width, t_depth);
    } else {
      t_key[depth - 1] = child->key;
      return child->advance_descend(t_key, t_width, t_depth);
    }
  }

  const node_t* retreat(key_t& t_key, size_t t_width, size_t t_depth) const {
    if (parent == nullptr)
      return nullptr;

    const node_t* child = parent->prev_child(key, t_width);
    if (child == nullptr) {
      return parent->retreat(t_key, t_width, t_depth);
    } else {
      t_key[depth - 1] = child->key;
      return child->retreat_descend(t_key, t_width, t_depth);
    }
  }

  const node_t* retreat_descend(key_t& t_key, size_t t_width,
                                size_t t_depth) const {
    if (is_leaf)
      return this;

    const node_t* child = last_child(t_width);

    if (child == nullptr) {
      return retreat(t_key, t_width, t_depth);
    } else {
      t_key[depth - 1] = child->key;
      return child->retreat_descend(t_key, t_width, t_depth);
    }
  }

  uint8_t key;
  uint8_t depth;
  void* data;
  bool is_leaf;
  node_t* parent;
};

template<typename reflog>
class rt_reflog_it : public std::iterator<std::forward_iterator_tag, reflog> {
 public:
  typedef radix_tree_node<reflog> node_t;
  typedef byte_string key_t;
  typedef reflog value_type;
  typedef rt_reflog_it<reflog> self_type;
  typedef reflog& reference;
  typedef reflog* pointer;

  rt_reflog_it()
      : width_(0),
        depth_(0),
        node_(nullptr) {

  }

  rt_reflog_it(size_t width, size_t depth, const key_t& key, const node_t* node)
      : width_(width),
        depth_(depth),
        key_(key),
        node_(node) {
  }

  reference operator*() const {
    return *(node_->refs());
  }

  pointer operator->() const {
    return node_->refs();
  }

  bool operator!=(const self_type& other) const {
    return node_ != other.node_;
  }

  bool operator==(const self_type& other) const {
    return node_ == other.node_;
  }

  const self_type& operator++() {
    if (node_ != nullptr)
      node_ = node_->advance(key_, width_, depth_);
    return *this;
  }

  self_type operator++(int) {
    self_type copy(*this);
    ++(*this);
    return copy;
  }

  key_t const& key() const {
    return key_;
  }

  const node_t* node() const {
    return node_;
  }

  self_type& set_node(const node_t* node) {
    node_ = node;
    return *this;
  }

 private:
  size_t width_;
  size_t depth_;
  key_t key_;
  const node_t* node_;
};

template<typename reflog>
class rt_reflog_range_result {
 public:
  typedef reflog value_type;
  typedef rt_reflog_it<reflog> const_iterator;
  typedef rt_reflog_it<reflog> iterator;

  rt_reflog_range_result(const iterator& lb, const iterator& ub)
      : begin_(lb),
        end_(ub) {
  }

  rt_reflog_range_result(const rt_reflog_range_result<reflog>& other)
      : begin_(other.begin_),
        end_(other.end_) {
  }

  const iterator begin() const {
    return begin_;
  }

  const iterator end() const {
    return end_;
  }

  size_t count() const {
    return std::accumulate(begin_, end_, static_cast<size_t>(0),
                           [](size_t count, reflog& refs) {
                             return count + 1;
                           });
  }
 private:
  iterator begin_;
  iterator end_;
};

template<typename reflog>
class radix_tree {
 public:
  typedef radix_tree_node<reflog> node_t;
  typedef byte_string key_t;
  typedef typename reflog::value_type value_t;

  typedef rt_reflog_it<reflog> iterator;
  typedef rt_reflog_range_result<reflog> rt_reflog_result;
  typedef flattened_container<rt_reflog_range_result<reflog>> rt_result;
  typedef typename rt_result::iterator range_iterator;

  radix_tree(size_t depth, size_t width)
      : width_(width),
        depth_(depth),
        root_(new node_t(0, width, 0, nullptr)) {
  }

  inline size_t width() const {
    return width_;
  }

  inline size_t depth() const {
    return depth_;
  }

  template<typename ... ARGS>
  reflog*& get_or_create(const key_t& key, ARGS&& ... args) {
    node_t* node = root_;
    for (size_t d = 0; d < depth_ - 1; d++) {
      node_t* child = nullptr;
      if ((child = atomic::load(&(node->children()[key[d]]))) == nullptr) {
        // Try & allocate child node
        child = new node_t(key[d], width_, d + 1, node);
        node_t* expected = nullptr;

        // If thread was not successful in swapping newly allocated memory,
        // then it should de-allocate memory, and accept whatever the
        // successful thread allocated as the de-facto storage for child node.
        if (!atomic::strong::cas(&(node->children()[key[d]]), &expected,
                                 child)) {
          delete child;
          child = expected;
        }
      }
      // child is definitely allocated now
      node = child;
    }

    // Reached leaf
    size_t d = depth_ - 1;
    node_t* child = nullptr;
    if ((child = atomic::load(&(node->children()[key[d]]))) == nullptr) {
      // Try & allocate child node
      child = new node_t(key[d], width_, d + 1, node, true,
                         std::forward<ARGS>(args)...);
      node_t* expected = nullptr;

      // If thread was not successful in swapping newly allocated memory,
      // then it should de-allocate memory, and accept whatever the
      // successful thread allocated as the de-facto storage for child node.
      if (!atomic::strong::cas(&(node->children()[key[d]]), &expected, child)) {
        delete child;
        child = expected;
      }
    }

    return child->refs();
  }

  template<typename ... ARGS>
  reflog*& insert(const key_t& key, const value_t& value, ARGS&& ... args) {
    reflog*& refs = get_or_create(key, std::forward<ARGS>(args)...);
    refs->push_back(value);
    return refs;
  }

  reflog const* get(const key_t& key) const {
    node_t* node = root_;
    size_t d;
    for (d = 0; d < depth_; d++) {
      node_t* child = atomic::load(&(node->children()[key[d]]));
      if (child == nullptr)
        return nullptr;
      node = child;
    }
    return node->refs();
  }

  reflog* operator[](const key_t& key) {
    node_t* node = root_;
    size_t d;
    for (d = 0; d < depth_; d++) {
      node_t* child = atomic::load(&(node->children()[key[d]]));
      if (child == nullptr)
        return nullptr;
      node = child;
    }
    return node->refs();
  }

  iterator upper_bound(const key_t& key) const {
    auto ub = __upper_bound(key);
    return iterator(width_, depth_, ub.first, ub.second);
  }

  iterator lower_bound(const key_t& key) const {
    auto lb = __lower_bound(key);
    return iterator(width_, depth_, lb.first, lb.second);
  }

  rt_reflog_result range_lookup_reflogs(const key_t& begin,
                                        const key_t& end) const {
    iterator ibegin = upper_bound(begin);
    iterator iend = lower_bound(end);
    if (ibegin.node() == nullptr) {
      return rt_reflog_result(ibegin, ibegin);
    } else if (iend.node() == nullptr) {
      return rt_reflog_result(iend, iend);
    }
    return rt_reflog_result(ibegin, ++iend);
  }

  rt_result range_lookup(const key_t& begin, const key_t& end) const {
    return rt_result(range_lookup_reflogs(begin, end));
  }

  size_t approx_count(const key_t& begin, const key_t& end) const {
    return std::accumulate(upper_bound(begin), ++lower_bound(end),
                           static_cast<size_t>(0),
                           [](size_t count, reflog& val) {
                             return count + val.size();
                           });
  }

  std::string to_string() const {
    const void *addr = static_cast<const void*>(this);
    std::stringstream ss;
    ss << addr;
    return ss.str();
  }

 private:
  // First leaf node <= key
  std::pair<key_t, const node_t*> __lower_bound(const key_t& key) const {
    std::pair<key_t, const node_t*> ret(key, root_);
    size_t d;
    for (d = 0; d < depth_; d++) {
      node_t* child = atomic::load(&(ret.second->children()[key[d]]));
      if (child == nullptr)
        break;
      ret.second = child;
    }

    if (d != depth_) {
      // ret.second now points to the parent where the search terminated
      // and key[d] is the failed child key

      // Obtain the next valid child
      const node_t* child = ret.second->prev_child(key[d], width_);
      if (child == nullptr) {       // There are no valid children of ret.second
        ret.second = ret.second->retreat(ret.first, width_, depth_);
      } else {  // There is a valid child of ret.second
        ret.first[d] = child->key;
        ret.second = child->retreat_descend(ret.first, width_, depth_);
      }
    }

    return ret;
  }

  // First leaf node >= key
  std::pair<key_t, const node_t*> __upper_bound(const key_t& key) const {
    std::pair<key_t, const node_t*> ret(key, root_);
    size_t d;
    for (d = 0; d < depth_; d++) {
      node_t* child = atomic::load(&(ret.second->children()[key[d]]));
      if (child == nullptr)
        break;
      ret.second = child;
    }

    if (d < depth_) {
      // ret.second now points to the parent where the search terminated
      // and key[d] is the failed child key

      // Obtain the next valid child
      const node_t* child = ret.second->next_child(key[d], width_);
      if (child == nullptr)        // There are no valid children of ret.second
        ret.second = ret.second->advance(ret.first, width_, depth_);
      else {  // There is a valid child of ret.second
        ret.first[d] = child->key;
        ret.second = child->advance_descend(ret.first, width_, depth_);
      }
    }

    return ret;
  }

  size_t width_;
  size_t depth_;
  node_t* root_;
};

typedef radix_tree<reflog> radix_index;

}
}

#endif /* CONFLUO_CONTAINER_RADIX_TREE_H_ */

#ifndef DIALOG_RADIX_TREE_H_
#define DIALOG_RADIX_TREE_H_

#include "atomic.h"
#include "byte_string.h"
#include "exceptions.h"
#include "flatten.h"

namespace dialog {
namespace index {

typedef monolog::monolog_exp2<uint64_t, 24> reflog;

class radix_tree;

struct radix_tree_node {
  typedef atomic::type<radix_tree_node*> child_t;
  typedef radix_tree_node node_t;
  typedef byte_string key_t;

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

  radix_tree_node(uint8_t node_key, size_t node_depth,
                  radix_tree_node* node_parent)
      : key(node_key),
        depth(node_depth),
        is_leaf(true),
        parent(node_parent) {
    data = new reflog;
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
    int16_t cur_key = width;
    const node_t* child = nullptr;
    while (cur_key > 0
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
    while (cur_key > 0
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
      t_key[child->depth] = child->key;
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
      t_key[child->depth] = child->key;
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
      t_key[child->depth] = child->key;
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
      t_key[child->depth] = child->key;
      return child->retreat_descend(t_key, t_width, t_depth);
    }
  }

  uint8_t key;
  uint8_t depth;
  void* data;
  bool is_leaf;
  radix_tree_node* parent;
};

class rt_reflog_it : public std::iterator<std::forward_iterator_tag, reflog> {
 public:
  typedef radix_tree_node node_t;
  typedef byte_string key_t;
  typedef rt_reflog_it self_type;

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

  rt_reflog_it(const self_type& other)
      : width_(other.width_),
        depth_(other.depth_),
        key_(other.key_),
        node_(other.node_) {
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
    assert(node_->is_leaf);
    return *this;
  }

  self_type operator++(int) {
    self_type copy(*this);
    ++(*this);
    return copy;
  }

 private:
  size_t width_;
  size_t depth_;
  key_t key_;
  const radix_tree_node* node_;
};

class rt_reflog_range_result {
 public:
  typedef rt_reflog_it const_iterator;
  typedef rt_reflog_it iterator;

  rt_reflog_range_result(const iterator& lb, const iterator& ub)
      : begin_(lb),
        end_(ub) {
    ++end_;
  }

  rt_reflog_range_result(const rt_reflog_range_result& other)
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

class radix_tree {
 public:
  typedef radix_tree_node node_t;
  typedef byte_string key_t;
  typedef uint64_t value_t;

  typedef rt_reflog_it iterator;
  typedef flattened_container<rt_reflog_range_result> rt_range_result;
  typedef rt_range_result::iterator range_iterator;

  radix_tree(size_t depth, size_t width)
      : width_(width),
        depth_(depth),
        root_(new radix_tree_node(0, width, 0, nullptr)) {
  }

  inline size_t width() const {
    return width_;
  }

  inline size_t depth() const {
    return depth_;
  }

  void insert(const key_t& key, const value_t& value) {
    node_t* node = root_;
    for (size_t d = 0; d < depth_ - 1; d++) {
      node_t* child = nullptr;
      if ((child = atomic::load(&(node->children()[key[d]]))) == nullptr) {
        // Try & allocate child node
        child = new node_t(key[d], width_, d, node);
        radix_tree_node* expected = nullptr;

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
      child = new node_t(key[d], d, node);
      node_t* expected = nullptr;

      // If thread was not successful in swapping newly allocated memory,
      // then it should de-allocate memory, and accept whatever the
      // successful thread allocated as the de-facto storage for child node.
      if (!atomic::strong::cas(&(node->children()[key[d]]), &expected, child)) {
        delete child;
        child = expected;
      }
    }
    child->refs()->push_back(value);
  }

  const reflog* at(const key_t& key) const {
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

  rt_reflog_range_result range_lookup_reflogs(const key_t& begin,
                                              const key_t& end) const {
    return rt_reflog_range_result(upper_bound(begin), lower_bound(end));
  }

  rt_range_result range_lookup(const key_t& begin, const key_t& end) const {
    return rt_range_result(range_lookup_reflogs(begin, end));
  }

  size_t approx_count(const key_t& begin, const key_t& end) {
    return std::accumulate(upper_bound(begin), ++lower_bound(end),
                           static_cast<size_t>(0),
                           [](size_t count, reflog& val) {
                             return count + val.size();
                           });
  }

 private:
  // First leaf node <= key
  std::pair<key_t, const node_t*> __lower_bound(const key_t& key) const {
    std::pair<key_t, const node_t*> ret(key, nullptr);
    ret.second = root_;
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
      if (child == nullptr)  // There are no valid children of ret.second
        ret.second = ret.second->retreat(ret.first, width_, depth_);
      else {  // There is a valid child of ret.second
        ret.first[child->depth] = child->key;
        ret.second = child->retreat_descend(ret.first, width_, depth_);
      }
    }

    return ret;
  }

  // First leaf node >= key
  std::pair<key_t, const node_t*> __upper_bound(const key_t& key) const {
    std::pair<key_t, const node_t*> ret(key, nullptr);
    ret.second = root_;
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
      if (child == nullptr)  // There are no valid children of ret.second
        ret.second = ret.second->advance(ret.first, width_, depth_);
      else {  // There is a valid child of ret.second
        ret.first[child->depth] = child->key;
        ret.second = child->advance_descend(ret.first, width_, depth_);
      }
    }

    return ret;
  }

  size_t width_;
  size_t depth_;
  radix_tree_node* root_;
};

}
}

#endif /* DIALOG_RADIX_TREE_H_ */

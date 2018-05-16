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

/**
 * A node in the radix tree
 */
template<typename reflog>
struct radix_tree_node {
  /** The radix node type */
  typedef radix_tree_node<reflog> node_t;
  /** The child type */
  typedef atomic::type<node_t*> child_t;
  /** The key */
  typedef byte_string key_t;

  /**
   * Constructs a new node from the specified arguments
   *
   * @tparam ARGS Template for arguments to the reflog constructor
   * @param node_key The node key
   * @param node_width The node width
   * @param node_depth The node depth
   * @param node_parent The node parent
   * @param args The arguments for the reflog constructor
   */
  template<typename ... ARGS>
  radix_tree_node(uint8_t node_key, size_t node_width, size_t node_depth,
                  node_t* node_parent, bool, ARGS&& ... args)
      : key(node_key),
        depth(node_depth),
        is_leaf(true),
        parent(node_parent) {
    data = new reflog(std::forward<ARGS>(args)...);
  }

  /**
   * Constructs a new radix tree node from the specified parameters
   *
   * @param node_key The node key
   * @param node_width The node width
   * @param node_depth The node depth
   * @param node_parent The node parent
   */
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

  /**
   * Deletes the radix tree node
   */
  ~radix_tree_node() {
    if (is_leaf)
      delete refs();
    else
      delete[] children();
  }

  /**
   * Gets a reference to the reflog of the data
   *
   * @return The reflog reference containing the data
   */
  inline reflog*& refs() {
    return reinterpret_cast<reflog*&>(data);
  }

  /**
   * Gets a constant reference to the reflog containing the data
   *
   * @return A constant reference to the reflog containing the data
   */
  inline reflog* const & refs() const {
    return reinterpret_cast<reflog* const &>(data);
  }

  /**
   * Gets the children of the node
   *
   * @return Reference to the children
   */
  inline child_t*& children() {
    return reinterpret_cast<child_t*&>(data);
  }

  /**
   * Gets a constant reference to the children
   *
   * @return A constant pointer to the children
   */
  inline child_t* const & children() const {
    return reinterpret_cast<child_t* const &>(data);
  }

  /**
   * Gets the first child of the node
   *
   * @param width The width of a child node
   *
   * @return A pointer to the first child
   */
  const node_t* first_child(size_t width) const {
    size_t cur_key = 0;
    const node_t* child = nullptr;
    while (cur_key < width
        && (child = atomic::load(&(children()[cur_key]))) == nullptr) {
      ++cur_key;
    }
    return child;
  }

  /**
   * Gets the last child of the node
   *
   * @param width The width of the child node
   *
   * @return A pointer to the child
   */
  const node_t* last_child(size_t width) const {
    int16_t cur_key = width - 1;
    const node_t* child = nullptr;
    while (cur_key >= 0
        && (child = atomic::load(&(children()[cur_key]))) == nullptr) {
      --cur_key;
    }
    return child;
  }

  /**
   * Gets the next child of the node
   *
   * @param key The key
   * @param width The width
   *
   * @return node_t
   */
  const node_t* next_child(uint8_t key, size_t width) const {
    size_t cur_key = key + 1;
    const node_t* child = nullptr;
    while (cur_key < width
        && (child = atomic::load(&(children()[cur_key]))) == nullptr) {
      ++cur_key;
    }
    return child;
  }

  /**
   * Gets the previous child for the given node key and width
   *
   * @param key The node key
   * @param width The node width
   *
   * @return A pointer to the child node (nullptr if none were found).
   */
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

  /**
   * Advance the traversal based on the current key, width, and depth.
   *
   * @param t_key The current key.
   * @param t_width The current width.
   * @param t_depth The current depth.
   *
   * @return Pointer to the advanced node.
   */
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

  /**
   * Descend one level into the tree and advance the traversal, based on
   * current key, width and depth.
   *
   *
   * @param t_key The current key.
   * @param t_width The current width.
   * @param t_depth The current depth.
   *
   * @return Pointer to the advanced node.
   */
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

  /**
   * Retreat the traversal based on current key, width and depth,
   *
   * @param t_key The current key.
   * @param t_width The current width.
   * @param t_depth The current depth.
   *
   * @return Pointer to the retreated node.
   */
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

  /**
   * Descend one level into the tree and retreat the traversal, based on current
   * key, width and depth.
   *
   * @param t_key The current key.
   * @param t_width The current width.
   * @param t_depth The current depth.
   *
   * @return Pointer to the retreated node.
   */
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

  /** Key of the radix tree node */
  uint8_t key;
  /** Depth of the radix tree node */
  uint8_t depth;
  /** Data that the node contains */
  void* data;
  /** Whether the node is a leaf node */
  bool is_leaf;
  /** The parent of the radix tree node */
  node_t* parent;
};

/**
 * Iterator over reflogs stored in the radix tree; traverses the tree in order.
 */
template<typename reflog>
class rt_reflog_it : public std::iterator<std::forward_iterator_tag, reflog> {
 public:
  /** The node type */
  typedef radix_tree_node<reflog> node_t;
  /** The key */
  typedef byte_string key_t;
  /** The value type */
  typedef reflog value_type;
  /** The self reflog type */
  typedef rt_reflog_it<reflog> self_type;
  /** The reflog reference */
  typedef reflog& reference;
  /** The reflog pointer */
  typedef reflog* pointer;

  /**
   * Default constructor.
   */
  rt_reflog_it()
      : width_(0),
        depth_(0),
        node_(nullptr) {

  }

  /**
   * Constructor to initialize reflog iterator over the radix tree with given
   * width, depth key and node.
   *
   * @param width The width
   * @param depth The depth
   * @param key The key
   * @param node The node
   */
  rt_reflog_it(size_t width, size_t depth, const key_t& key, const node_t* node)
      : width_(width),
        depth_(depth),
        key_(key),
        node_(node) {
  }

  /**
   * operator*
   *
   * @return Iterator data
   */
  reference operator*() const {
    return *(node_->refs());
  }

  /**
   * operator->
   *
   * @return Reference to iterator data.
   */
  pointer operator->() const {
    return node_->refs();
  }

  /**
   * operator!=
   *
   * @param other Another iterator.
   *
   * @return Returns true if the iterator does not equal the other iterator,
   * false otherwise.
   */
  bool operator!=(const self_type& other) const {
    return node_ != other.node_;
  }

  /**
   * operator==
   *
   * @param other Another iterator.
   *
   * @return Returns true if the iterator equals the other iterator,
   * false otherwise.
   */
  bool operator==(const self_type& other) const {
    return node_ == other.node_;
  }

  /**
   * operator++ (prefix)
   *
   * @return Updated iterator.
   */
  const self_type& operator++() {
    if (node_ != nullptr)
      node_ = node_->advance(key_, width_, depth_);
    return *this;
  }

  /**
   * operator++ (postfix)
   *
   *
   * @return Updated iterator
   */
  self_type operator++(int) {
    self_type copy(*this);
    ++(*this);
    return copy;
  }

  /**
   * Get the iterator key.
   *
   * @return The iterator key.
   */
  key_t const& key() const {
    return key_;
  }

  /**
   * Get the iterator node.
   *
   * @return The iterator node.
   */
  const node_t* node() const {
    return node_;
  }

  /**
   * Set the node for the iterator.
   *
   * @param node Pointer to th node.
   *
   * @return Updated iterator.
   */
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

/**
 * Result of a lookup on radix tree, as a container of reflogs. Only yields
 * values through iterators.
 */
template<typename reflog>
class rt_reflog_range_result {
 public:
  /** The value type */
  typedef reflog value_type;
  /** The constant iterator type */
  typedef rt_reflog_it<reflog> const_iterator;
  /** The iterator type */
  typedef rt_reflog_it<reflog> iterator;

  /**
   * Constructor to initialize the result with a lower bound iterator and
   * an upper bound iterator for the range within the radix tree.
   *
   * @param lb The lower bound iterator in the radix tree.
   * @param ub The upper bound iterator in the radix tree.
   */
  rt_reflog_range_result(const iterator& lb, const iterator& ub)
      : begin_(lb),
        end_(ub) {
  }

  /**
   * Copy constructor.
   *
   * @param other Another radix tree lookup result
   */
  rt_reflog_range_result(const rt_reflog_range_result<reflog>& other)
      : begin_(other.begin_),
        end_(other.end_) {
  }

  /**
   * Get the begin iterator
   *
   * @return iterator The begin iterator.
   */
  const iterator begin() const {
    return begin_;
  }

  /**
   * Get the end iterator.
   *
   * @return iterator The end iterator
   */
  const iterator end() const {
    return end_;
  }

  /**
   * Get an estimate of the number of elements in the range.
   *
   * @return The estimate of the number of elements in the range.
   */
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

/**
 * Radix Tree class. The radix tree uses a fixed-depth, fixed-width (k)-ary
 * tree to index reflogs based on a byte_array key.
 *
 * @tparam reflog The reflog type.
 */
template<typename reflog>
class radix_tree {
 public:
  /** The node type */
  typedef radix_tree_node<reflog> node_t;
  /** The key */
  typedef byte_string key_t;
  /** The value type */
  typedef typename reflog::value_type value_t;

  /** The iterator type for reflog */
  typedef rt_reflog_it<reflog> iterator;
  /** The range result type */
  typedef rt_reflog_range_result<reflog> rt_reflog_result;
  /** The result type */
  typedef flattened_container<rt_reflog_range_result<reflog>> rt_result;
  /** The range iterator type */
  typedef typename rt_result::iterator range_iterator;

  /**
   * Constructor to initialize the radix tree with a given depth and width
   *
   * @param depth The radix tree depth
   * @param width The radix tree node width
   */
  radix_tree(size_t depth, size_t width)
      : width_(width),
        depth_(depth),
        root_(new node_t(0, width, 0, nullptr)) {
  }

  /**
   * Get the width of each node.
   *
   * @return The width of each node.
   */
  inline size_t width() const {
    return width_;
  }

  /**
   * Get the depth of the tree.
   *
   * @return The depth of the tree.
   */
  inline size_t depth() const {
    return depth_;
  }

  /**
   * Get a leaf node (reflog) for the specified key, and create one if such a
   * node does not exist
   *
   * @tparam ARGS Template of arguments to reflog constructor.
   * @param key The key for the reflog.
   * @param args The args for the reflog constructor.
   *
   * @return The reflog corresponding to the given key.
   */
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

  /**
   * Insert a new (key, value) pair into the radix tree.
   *
   * @tparam ARGS Template for arguments to the reflog constructor.
   * @param key The key to insert.
   * @param value The value to insert.
   * @param args The arguments to the reflog constructor.
   *
   * @return The reflog to which the value was inserted.
   */
  template<typename ... ARGS>
  reflog*& insert(const key_t& key, const value_t& value, ARGS&& ... args) {
    reflog*& refs = get_or_create(key, std::forward<ARGS>(args)...);
    refs->push_back(value);
    return refs;
  }

  /**
   * Get the reflog corresponding to a given key; returns null if the key has
   * not been indexed. Unsafe to modify the returned reflog.
   *
   * @param key The key to lookup.
   *
   * @return The reflog corresponding to the key.
   */
  reflog* get_unsafe(const key_t& key) const {
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

  /**
   * Get the reflog corresponding to a given key; returns null if the key has
   * not been indexed.
   *
   * @param key The key to lookup.
   *
   * @return The reflog corresponding to the key.
   */
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

  /**
   * operator[]
   *
   * @param key The to lookup key.
   *
   * @return The reflog corresponding to the key.
   */
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

  /**
   * Get the iterator to reflog corresponding to the smallest key larger than or
   * equal to the given key in the radix tree.
   *
   * @param key The key.
   *
   * @return Iterator to reflog corresponding to the smallest key larger than or
   * equal to the given key in the radix tree.
   */
  iterator upper_bound(const key_t& key) const {
    auto ub = __upper_bound(key);
    return iterator(width_, depth_, ub.first, ub.second);
  }

  /**
   * Get the iterator to reflog corresponding to the largest key smaller than or
   * equal to the given key in the radix tree.
   *
   * @param key The key.
   *
   * @return Iterator to reflog corresponding to the largest key smaller than or
   * equal to the given key in the radix tree.
   */
  iterator lower_bound(const key_t& key) const {
    auto lb = __lower_bound(key);
    return iterator(width_, depth_, lb.first, lb.second);
  }

  /**
   * Lookup all reflogs that lie within a given range in the radix tree.
   *
   * @param begin The begin key for the range.
   * @param end The end key for the range.
   *
   * @return A container of reflogs corresponding to the range.
   */
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

  /**
   * Lookup all values that lie within a given range in the radix tree.
   *
   * @param begin The begin key for the range.
   * @param end The end key for the range.
   *
   * @return A container of values corresponding to the range.
   */
  rt_result range_lookup(const key_t& begin, const key_t& end) const {
    return rt_result(range_lookup_reflogs(begin, end));
  }

  /**
   * An approximate count (potentially inconsistent) of the values in the
   * radix tree that lie within a particular range.
   *
   * @param begin The begin key for the range.
   * @param end The end key for the range.
   *
   * @return The approximate count.
   */
  size_t approx_count(const key_t& begin, const key_t& end) const {
    return std::accumulate(upper_bound(begin), ++lower_bound(end),
                           static_cast<size_t>(0),
                           [](size_t count, reflog& val) {
                             return count + val.size();
                           });
  }

  /**
   * Get the string corresponding to the radix tree address.
   *
   * @return The string corresponding to the radix tree address.
   */
  std::string to_string() const {
    const void *addr = static_cast<const void*>(this);
    std::stringstream ss;
    ss << addr;
    return ss.str();
  }

 private:
  /**
   * Get the first node whose key is smaller than or equal to specified key.
   * @param key The given key.
   * @return The node and its corresponding key.
   */
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

  /**
   * Get the first node whose key is larger than or equal to specified key.
   * @param key The given key.
   * @return The node and its corresponding key.
   */
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

/** The radix index */
typedef radix_tree<reflog> radix_index;

}
}

#endif /* CONFLUO_CONTAINER_RADIX_TREE_H_ */

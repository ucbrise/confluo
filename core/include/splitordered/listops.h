#ifndef SPLITORDERED_LISTOPS_H_
#define SPLITORDERED_LISTOPS_H_

#include "defs.h"

namespace splitordered {

// Lock free operations on lists. Only support two operations:
// - insert
// - find
template<class data_type>
class list_ops {
 public:
  typedef hash_entry<data_type> node_t;
  typedef node_t* node_ptr_t;

  // Atomically inserts a node in the list; if the key corresponding to the node
  // already exists, fails and returns.
  static bool insert(node_ptr_t *head, node_ptr_t node, node_ptr_t *ocur) {
    so_key_t key = node->key;

    while (1) {
      node_ptr_t *lprev;
      node_ptr_t cur;

      if (find(head, key, NULL, &lprev, &cur, NULL)) {  // needs to set cur/prev
        if (ocur)
          *ocur = cur;
        return false;
      }
      node->next = cur;
      if (CAS(lprev, node->next, node) == cur) {
        if (ocur)
          *ocur = cur;
        return true;
      }
    }
  }

  static bool upsert(node_ptr_t *head, so_key_t key, data_type value,
                     data_type *old_value) {
    while (1) {
      node_ptr_t *lprev;
      node_ptr_t cur;
      data_type cur_value;
      if (find(head, key, &cur_value, &lprev, &cur, NULL)) {  // needs to set cur/prev
        bool success = false;
        while (!success && cur_value < value) {
          success = std::atomic_compare_exchange_weak(&cur->value, &cur_value, value);
        }
        if (old_value)
          *old_value = cur_value;
        return success;
      }

      node_t *node = new node_t;
      node->key = key;
      node->value = value;
      node->next = cur;
      if (CAS(lprev, node->next, node) == cur) {
        return false;
      }
      delete node;
    }
  }

  // Atomically finds a given key in the list, while populating the corresponding value,
  // the containing node, and the nodes immediately before and after the containing node
  static bool find(node_ptr_t *head, so_key_t key, data_type* oval,
                   node_ptr_t **oprev, node_ptr_t *ocur, node_ptr_t *onext) {
    so_key_t ckey;
    data_type cval;
    node_ptr_t *prev = NULL;
    node_ptr_t cur = NULL;
    node_ptr_t next = NULL;

    while (1) {
      prev = head;
      cur = *prev;
      while (1) {
        if (cur == NULL) {
          if (oprev)
            *oprev = prev;
          if (ocur)
            *ocur = cur;
          if (onext)
            *onext = next;
          if (oval)
            *oval = cval;
          return false;
        }
        next = cur->next;
        ckey = cur->key;
        cval = cur->value;

        // this means someone mucked with the list; start over
        if (*prev != cur) {
          break;
        }

        // if current key > key, the key isn't in the list;
        // if current key == key, the key IS in the list
        if (ckey >= key) {
          if (oprev)
            *oprev = prev;
          if (ocur)
            *ocur = cur;
          if (onext)
            *onext = next;
          if (oval)
            *oval = cval;
          return (ckey == key);
        }

        // but if current key < key, the we don't know yet, keep looking
        prev = &(cur->next);
        cur = next;
      }
    }
  }
};

}

#endif /* SPLITORDERED_LISTOPS_H_ */

#ifndef SPLITORDERED_LISTOPS_H_
#define SPLITORDERED_LISTOPS_H_

#include "defs.h"

namespace splitordered {

template<class data_type>
class list_ops {
 public:
  typedef hash_entry<data_type>* node_ptr_t;

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
        if (*prev != cur) {
          break;  // this means someone mucked with the list; start over
        }

        if (ckey >= key) {  // if current key > key, the key isn't in the list; if current key == key, the key IS in the list
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

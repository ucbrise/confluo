#ifndef SPLITORDERED_LISTOPS_H_
#define SPLITORDERED_LISTOPS_H_

#include "defs.h"

namespace splitordered {

template<class data_type>
class list_ops {
 public:
  static bool insert(marked_ptr_t *head, hash_entry<data_type> *node,
                     marked_ptr_t *ocur) {
    so_key_t key = node->key;

    while (1) {
      marked_ptr_t *lprev;
      marked_ptr_t cur;

      if (find(head, key, NULL, &lprev, &cur, NULL)) {  // needs to set cur/prev
        if (ocur)
          *ocur = cur;
        return false;
      }
      node->next = CONSTRUCT(0, cur);
      if (CAS(lprev, node->next, CONSTRUCT(0, node)) == CONSTRUCT(0, cur)) {
        if (ocur)
          *ocur = cur;
        return true;
      }
    }
  }

  static bool remove(marked_ptr_t *head, so_key_t key) {
    while (1) {
      marked_ptr_t *lprev;
      marked_ptr_t lcur;
      marked_ptr_t lnext;

      if (find(head, key, NULL, &lprev, &lcur, &lnext))
        return false;

      if (CAS(&PTR_OF(lcur)->next, CONSTRUCT(0, lnext),
          CONSTRUCT(1, lnext)) != CONSTRUCT(0, lnext))
        continue;

      if (CAS(lprev, CONSTRUCT(0, lcur),
          CONSTRUCT(0, lnext)) == CONSTRUCT(0, lcur))
        free(PTR_OF(lcur));
      else
        find(head, key, NULL, NULL, NULL, NULL);  // needs to set cur/prev/next

      return true;
    }
  }

  static bool find(marked_ptr_t *head, so_key_t key, data_type* oval,
                   marked_ptr_t **oprev, marked_ptr_t *ocur,
                   marked_ptr_t *onext) {
    so_key_t ckey;
    data_type cval;
    marked_ptr_t *prev = NULL;
    marked_ptr_t cur = UNINITIALIZED;
    marked_ptr_t next = UNINITIALIZED;

    while (1) {
      prev = head;
      cur = *prev;
      while (1) {
        if (PTR_OF(cur) == NULL) {
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
        next = PTR_OF(cur)->next;
        ckey = PTR_OF(cur)->key;
        cval = PTR_OF(cur)->value;
        if (*prev != CONSTRUCT(0, cur)) {
          break;  // this means someone mucked with the list; start over
        }
        if (!MARK_OF(next)) {  // if next pointer is not marked
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
          prev = &(PTR_OF(cur)->next);
        } else {
          if (CAS(prev, CONSTRUCT(0, cur),
              CONSTRUCT(0, next)) == CONSTRUCT(0, cur)) {
            free(PTR_OF(cur));
          } else {
            break;
          }
        }
        cur = next;
      }
    }
  }
};

}

#endif /* SPLITORDERED_LISTOPS_H_ */

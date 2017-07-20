#ifndef DIALOG_SEARCHABLE_LIST_H_
#define DIALOG_SEARCHABLE_LIST_H_

#include <cstdint>

namespace dialog {

template<typename T>
struct marked_ptr {
 public:
  marked_ptr(bool mark, uintptr_t ptr)
      : ptr_((ptr & ~static_cast<uintptr_t>(1)) | mark) {
  }

  marked_ptr()
      : ptr_(nullptr) {
  }

  bool is_marked() const {
    return ptr_ & 1;
  }

  T& operator*() {
    return *reinterpret_cast<T*>(ptr_ & ~static_cast<uintptr_t>(1));
  }

  T* operator->() {
    return reinterpret_cast<T*>(ptr_ & ~static_cast<uintptr_t>(1));
  }

  T* ptr() const {
    return reinterpret_cast<T*>(ptr_ & ~static_cast<uintptr_t>(1));
  }

  bool operator==(const marked_ptr<T>& other) const {
    return ptr_ == other.ptr_;
  }

  bool operator!=(const marked_ptr<T>& other) const {
    return ptr_ != other.ptr_;
  }

  marked_ptr<T> unmarked() const {
    return marked_ptr<T>(0, ptr_);
  }

  marked_ptr<T> marked() const {
    return marked_ptr<T>(1, ptr_);
  }

  bool is_uninitialized() const {
    return ptr_ == nullptr;
  }

 private:
  uintptr_t ptr_;
};

template<typename K, typename V>
struct searchable_list_node {
  typedef searchable_list_node<K, V> node_t;
  typedef marked_ptr<node_t> marked_ptr_t;

  K key;
  V value;

  searchable_list_node(const K& k, const V& v)
      : key(k),
        value(v) {
  }

  searchable_list_node()
      : key(),
        value() {
  }

  marked_ptr_t next;
};

template<typename K, typename V>
struct searchable_list {
 public:
  typedef searchable_list_node<K, V> node_t;
  typedef marked_ptr<node_t> marked_ptr_t;

  searchable_list(marked_ptr_t* head)
      : head_(head) {
  }

  bool find(const K& key, V& val, marked_ptr_t** oprev = nullptr,
            marked_ptr_t* ocur = nullptr, marked_ptr_t* onext = nullptr) {
    K ckey;
    marked_ptr_t *prev = nullptr;
    marked_ptr_t cur, next;

    while (true) {
      prev = head_;
      cur = *prev;
      while (true) {
        if (cur.ptr() == nullptr) {
          if (oprev)
            *oprev = prev;
          if (ocur)
            *ocur = cur;
          if (onext)
            *onext = next;
          return false;
        }

        next = cur->next;
        ckey = cur->key;
        val = cur->value;

        // this means someone mucked with the list; start over
        if (*prev != cur.unmarked())
          break;

        // if next pointer is not marked
        if (!next.marked()) {
          // if current key > key, the key isn't in the list;
          // if current key == key, the key IS in the list
          if (ckey >= key) {
            if (oprev)
              *oprev = prev;
            if (ocur)
              *ocur = cur;
            if (onext)
              *onext = next;
            return ckey == key;
          }
        } else {
          marked_ptr_t expected = cur.unmarked();
          if (atomic::c11::strong::cas(prev, &expected, next.unmarked())) {
            delete cur.ptr();
          } else {
            break;
          }
        }
        cur = next;
      }
    }
  }

  bool insert(const K& key, const V& value, marked_ptr_t *ocur = nullptr) {
    node_t* node = new node_t(key, value);
    while (true) {
      marked_ptr_t *lprev;
      marked_ptr_t cur;
      V val;
      if (find(key, val, &lprev, &cur, nullptr)) {
        if (ocur)
          ocur = cur;
        return false;
      }
      node->next = cur.unmarked();
      if (atomic::c11::strong::cas(lprev, &node->next,
                                   marked_node_t(false, node))) {
        if (ocur)
          ocur = cur;
        return true;
      }
    }
  }

  bool remove(const K& key) {
    while (true) {
      marked_ptr_t *lprev;
      marked_ptr_t lcur;
      marked_ptr_t lnext;
      V val;

      if (find(key, val, &lprev, &lcur, &lnext))
        return false;

      marked_ptr_t expected = lnext.unmarked();
      if (!atomic::c11::strong::cas(&lcur->next, &expected, lnext.marked()))
        continue;
      expected = lcur.unmarked();
      if (atomic::c11::strong::cas(lprev, expected, lnext.unmarked()))
        delete lcur.ptr();
      else
        find(key, val);
      return true;
    }
  }

 private:
  marked_ptr_t *head_;
};

}

#endif /* DIALOG_SEARCHABLE_LIST_H_ */

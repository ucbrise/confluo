#ifndef DATASTORE_OBJECT_LOG_H_
#define DATASTORE_OBJECT_LOG_H_

#include "object.h"
#include "monolog.h"
#include "concurrency_control.h"

namespace datastore {

struct empty_aux {
  template<typename T>
  void update(const T& obj) {
    return;
  }

  void update(const uint8_t* data, size_t len) {
    return;
  }
};

struct in_memory {
  in_memory() = default;

  void set_path(const std::string& p) {
    path = p;
  }

  std::string path;
  monolog::monolog_relaxed_linear<uint8_t> data_log_;
  monolog::monolog_linear_base<object_ptr_t, 8192, 16777216> ptr_log_;
};

struct persistent_relaxed {
  persistent_relaxed() = default;

  void set_path(const std::string& p) {
    path = p;
    data_log_.init("data", path);
    ptr_log_.init("ptrs", path);
  }

  std::string path;
  monolog::mmap_monolog_relaxed<uint8_t> data_log_;
  monolog::mmap_monolog_base<object_ptr_t> ptr_log_;
};

template<typename storage, typename concurrency_control, typename aux_data>
class log_store_base {
 public:
  typedef concurrency_control cc;

  log_store_base() = default;

  log_store_base(const std::string& path) {
    primary_.set_path(path);
  }

  object_ptr_t& write(uint64_t id, const uint8_t* data, size_t length,
                      uint64_t version) {
    object_ptr_t& p = primary_.ptr_log_[id];
    p.length = length;
    p.version = version;
    if (length > 0) {
      p.offset = primary_.data_log_.append(data, p.length);
      aux_.update(data, length);
    }
    return p;
  }

  template<typename T>
  object_ptr_t& write(uint64_t id, const T& data, uint64_t version) {
    object_ptr_t& p = primary_.ptr_log_[id];
    p.length = serializer<T>::size(data);
    p.version = version;
    p.offset = primary_.data_log_.reserve(p.length);
    serializer<T>::serialize(primary_.data_log_.ptr(p.offset), data);
    aux_.update(data);
    return p;
  }

  object_ptr_t& ptr(uint64_t id, uint64_t max_version) {
    uint64_t state;
    while (true) {
      state = cc::get_state(primary_.ptr_log_[id]);
      if (cc::is_valid(state)
          || primary_.ptr_log_.get(state).version >= max_version)
        break;
      id = state;
    }
    return primary_.ptr_log_[id];
  }

  const object_ptr_t cptr(uint64_t id, uint64_t max_version) const {
    uint64_t state;
    while (true) {
      object_ptr_t p = primary_.ptr_log_.get(id);
      state = cc::get_state(p);
      if (cc::is_valid(state)
          || primary_.ptr_log_.get(state).version >= max_version)
        break;
      id = state;
    }
    return primary_.ptr_log_.get(id);
  }

  bool read(uint64_t id, uint8_t* data, size_t& length,
            uint64_t max_version) const {
    object_ptr_t p = cptr(id, max_version);
    length = p.length;
    if (p.version < max_version && p.length != 0) {
      primary_.data_log_.read(p.offset, data, length);
      return true;
    }
    return false;
  }

  template<typename T>
  bool read(uint64_t id, T* data, uint64_t max_version) const {
    object_ptr_t p = cptr(id, max_version);
    if (p.version < max_version && p.length != 0) {
      deserializer<T>::deserialize(primary_.data_log_.ptr(p.offset), data);
      return true;
    }
    return false;
  }

 protected:
  storage primary_;
  aux_data aux_;
};

namespace dependent {

template<typename storage = in_memory,
    typename concurrency_control = read_stalled, typename aux_data = empty_aux>
class log_store : public log_store_base<storage, concurrency_control, aux_data> {
 public:
  typedef concurrency_control cc;

  log_store(concurrency_control& ctrl)
      : tail_(UINT64_C(0)),
        cc_(ctrl) {
  }

  log_store(concurrency_control& ctrl, const std::string& path)
      : log_store_base<storage, concurrency_control, aux_data>(path),
        tail_(UINT64_C(0)),
        cc_(ctrl) {
  }

  uint64_t append(uint8_t* data, size_t length) {
    uint64_t version = cc_.start_write_op();
    uint64_t id = atomic::faa(&tail_, UINT64_C(1));
    object_ptr_t& p = this->write(id, data, length, version);
    cc_.end_write_op(p, version);
    return id;
  }

  template<typename T>
  uint64_t append(const T& data) {
    uint64_t version = cc_.start_write_op();
    uint64_t id = atomic::faa(&tail_, UINT64_C(1));
    object_ptr_t& p = this->write(id, data, version);
    cc_.end_write_op(p, version);
    return id;
  }

  bool get(uint64_t id, uint8_t* data, size_t& length) const {
    if (id > atomic::load(&tail_))
      return false;
    uint64_t max_version = cc_.get_tail();
    return this->read(id, data, length, max_version);
  }

  template<typename T>
  bool get(uint64_t id, T* data) const {
    if (id >= atomic::load(&tail_))
      return false;
    uint64_t max_version = cc_.get_tail();
    return this->read(id, data, max_version);
  }

  bool update(uint64_t id, uint8_t* data, size_t length) {
    if (id >= atomic::load(&tail_))
      return false;
    while (true) {
      uint64_t max_version = cc_.get_tail();
      object_ptr_t& p = this->ptr(id, max_version);
      if (p.version < max_version && cc::start_update_op(p)) {
        uint64_t new_id = append(data, length);
        cc::end_update_op(p, new_id);
        return true;
      }
    }
  }

  template<typename T>
  bool update(uint64_t id, const T& data) {
    if (id >= atomic::load(&tail_))
      return false;
    while (true) {
      uint64_t max_version = cc_.get_tail();
      object_ptr_t& p = this->ptr(id, max_version);
      if (p.version < max_version && cc::start_update_op(p)) {
        cc::end_update_op(p, append(data));
        return true;
      }
    }
  }

  bool invalidate(uint64_t id) {
    return update(id, NULL, 0);
  }

 private:
  atomic::type<uint64_t> tail_;
  concurrency_control& cc_;
};

}

template<typename storage = in_memory,
    typename concurrency_control = read_stalled, typename aux_data = empty_aux>
class log_store : public log_store_base<storage, concurrency_control, aux_data> {
 public:
  typedef concurrency_control cc;

  log_store() = default;

  log_store(const std::string& path)
      : log_store_base<storage, concurrency_control, aux_data>(path) {
  }

  uint64_t append(uint8_t* data, size_t length) {
    uint64_t id = cc_.start_write_op();
    object_ptr_t& p = this->write(id, data, length, id);
    cc_.end_write_op(p, id);
    return id;
  }

  template<typename T>
  uint64_t append(const T& data) {
    uint64_t id = cc_.start_write_op();
    object_ptr_t& p = this->write(id, data, id);
    cc_.end_write_op(p, id);
    return id;
  }

  bool get(uint64_t id, uint8_t* data, size_t& length) const {
    uint64_t max_id = cc_.get_tail();
    if (id >= max_id)
      return false;
    return this->read(id, data, length, max_id);
  }

  template<typename T>
  bool get(uint64_t id, T* data) const {
    uint64_t max_id = cc_.get_tail();
    if (id > max_id)
      return false;
    return this->read(id, data, max_id);
  }

  bool update(uint64_t id, uint8_t* data, size_t length) {
    while (true) {
      uint64_t max_id = cc_.get_tail();
      if (id > max_id)
        return false;
      object_ptr_t& p = this->ptr(id, max_id);
      if (p.version < max_id && cc::start_update_op(p)) {
        uint64_t new_id = append(data, length);
        cc::end_update_op(p, new_id);
        return true;
      }
    }
  }

  template<typename T>
  bool update(uint64_t id, const T& data) {
    while (true) {
      uint64_t max_id = cc_.get_tail();
      if (id > max_id)
        return false;
      object_ptr_t& p = this->ptr(id, max_id);
      if (p.version < max_id && cc::start_update_op(p)) {
        cc::end_update_op(p, append(data));
        return true;
      }
    }
  }

  bool invalidate(uint64_t id) {
    return update(id, NULL, 0);
  }

 private:
  concurrency_control cc_;
};

}

#endif /* DATASTORE_OBJECT_LOG_H_ */

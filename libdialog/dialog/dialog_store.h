#ifndef DIALOG_DIALOG_STORE_H_
#define DIALOG_DIALOG_STORE_H_

#include <functional>
#include <numeric>

#include "concurrency_control.h"
#include "data_point.h"
#include "monolog.h"

namespace dialog {

struct empty_aux {
  template<typename T>
  void update(uint64_t id, const T& obj) {
    return;
  }

  void update(uint64_t id, const uint8_t* data, size_t len) {
    return;
  }
};

struct in_memory {
  in_memory() = default;

  void set_path(const std::string& p) {
    path = p;
  }

  void flush_ids(uint64_t start_id, size_t count) {
    // Do nothing
  }

  void flush_data(size_t offset, size_t len) {
    // Do nothing
  }

  std::string path;
  monolog::monolog_relaxed_linear<uint8_t, 65536, 1073741824> data_log_;
  monolog::monolog_linear_base<data_ptr_t, 65536, 16777216> ptr_log_;
};

struct durable {
  durable() = default;

  void set_path(const std::string& p) {
    path = p;
    data_log_.init("data", path);
    ptr_log_.init("ptrs", path);
  }

  void flush_ids(uint64_t start_id, size_t count) {
    ptr_log_.flush(start_id, count);
  }

  void flush_data(size_t offset, size_t len) {
    data_log_.flush(offset, len);
  }

  std::string path;
  monolog::mmap_monolog_relaxed<uint8_t, 65536, 1073741824> data_log_;
  monolog::mmap_monolog_base<data_ptr_t, 65536, 16777216> ptr_log_;
};

struct durable_relaxed {
  durable_relaxed() = default;

  void set_path(const std::string& p) {
    path = p;
    data_log_.init("data", path);
    ptr_log_.init("ptrs", path);
  }

  void flush_ids(uint64_t start_id, size_t count) {
    // Do nothing
  }

  void flush_data(size_t offset, size_t len) {
    // Do nothing
  }

  std::string path;
  monolog::mmap_monolog_relaxed<uint8_t, 65536, 1073741824> data_log_;
  monolog::mmap_monolog_base<data_ptr_t, 65536, 16777216> ptr_log_;
};

template<typename storage, typename concurrency_control, typename aux_data>
class dialog_store_base {
 public:
  typedef concurrency_control cc;

  dialog_store_base() = default;

  dialog_store_base(const std::string& path) {
    primary_.set_path(path);
  }

  data_ptr_t& write(uint64_t id, const uint8_t* data, size_t length,
                    uint64_t version) {
    data_ptr_t& p = primary_.ptr_log_[id];
    p.length = length;
    p.version = version;
    if (length > 0) {
      p.offset = primary_.data_log_.append(data, p.length);
      aux_.update(id, data, length);
    }
    return p;
  }

  template<typename T>
  data_ptr_t& write(uint64_t id, const T& data, uint64_t version) {
    data_ptr_t& p = primary_.ptr_log_[id];
    p.length = serializer<T>::size(data);
    p.version = version;
    p.offset = primary_.data_log_.reserve(p.length);
    serializer<T>::serialize(primary_.data_log_.ptr(p.offset), data);
    aux_.update(id, data);
    return p;
  }

  data_ptr_t& ptr(uint64_t id, uint64_t max_version) {
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

  const data_ptr_t cptr(uint64_t id, uint64_t max_version) const {
    uint64_t state;
    while (true) {
      data_ptr_t p = primary_.ptr_log_.get(id);
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
    data_ptr_t p = cptr(id, max_version);
    length = p.length;
    if (p.version < max_version && p.length != 0) {
      primary_.data_log_.read(p.offset, data, length);
      return true;
    }
    return false;
  }

  template<typename T>
  bool read(uint64_t id, T* data, uint64_t max_version) const {
    data_ptr_t p = cptr(id, max_version);
    if (p.version < max_version && p.length != 0) {
      deserializer<T>::deserialize(primary_.data_log_.cptr(p.offset), data);
      return true;
    }
    return false;
  }

 protected:
  storage primary_;
  aux_data aux_;
};

template<typename storage = in_memory,
    typename concurrency_control = read_stalled, typename aux_data = empty_aux>
class dialog_store : public dialog_store_base<storage, concurrency_control, aux_data> {
 public:
  typedef concurrency_control cc;

  dialog_store()
      : snapshot_tail_(UINT64_C(0)),
        snapshot_success_(false) {

  }

  dialog_store(const std::string& path)
      : dialog_store_base<storage, concurrency_control, aux_data>(path),
        snapshot_tail_(UINT64_C(0)),
        snapshot_success_(false) {
  }

  uint64_t append(const uint8_t* data, size_t length) {
    uint64_t id = cc_.start_write_op();
    data_ptr_t& p = this->write(id, data, length, id);
    cc_.init_object(p, id);
    this->primary_.flush_ids(id, 1);
    this->primary_.flush_data(p.offset, p.length);
    cc_.end_write_op(id);
    return id;
  }

  template<typename T>
  uint64_t append(const T& data) {
    uint64_t id = cc_.start_write_op();
    data_ptr_t& p = this->write(id, data, id);
    cc_.init_object(p, id);
    this->primary_.flush_ids(id, 1);
    this->primary_.flush_data(p.offset, p.length);
    cc_.end_write_op(id);
    return id;
  }

  uint64_t append_batch(const std::vector<uint8_t*>& data_batch,
                        std::vector<size_t>& length_batch) {
    uint64_t cnt = data_batch.size();
    uint64_t id = cc_.start_write_op(cnt);
    size_t size = 0;
    size_t off = 0;
    for (size_t i = 0; i < cnt; i++) {
      data_ptr_t& p = this->write(id, data_batch[i], length_batch[i], id);
      if (i == 0)
        off = p.offset;
      size += p.length;
      cc_.init_object(p, id);
    }
    this->primary_.flush_ids(id, cnt);
    this->primary_.flush_data(off, size);
    cc_.end_write_op(id, cnt);
    return id;
  }

  template<typename T>
  uint64_t append_batch(const std::vector<T>& data_batch) {
    uint64_t cnt = data_batch.size();
    uint64_t id = cc_.start_write_op(cnt);
    size_t size = 0;
    size_t off = 0;
    for (size_t i = 0; i < cnt; i++) {
      data_ptr_t& p = this->write(id, data_batch.at(i), id);
      if (i == 0)
        off = p.offset;
      size += p.length;
      cc_.init_object(p, id);
    }
    this->primary_.flush_ids(id, cnt);
    this->primary_.flush_data(off, size);
    cc_.end_write_op(id, cnt);
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

  bool update(uint64_t id, const uint8_t* data, size_t length) {
    while (true) {
      uint64_t max_id = cc_.get_tail();
      if (id > max_id)
        return false;
      data_ptr_t& p = this->ptr(id, max_id);
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
      data_ptr_t& p = this->ptr(id, max_id);
      if (p.version < max_id && cc::start_update_op(p)) {
        cc::end_update_op(p, append(data));
        return true;
      }
    }
  }

  bool invalidate(uint64_t id) {
    return update(id, NULL, 0);
  }

  size_t num_records() const {
    return cc_.get_tail();
  }

 private:
  uint64_t snapshot_tail_;
  bool snapshot_success_;
  concurrency_control cc_;
};

namespace append_only {

struct in_memory {
  in_memory() = default;

  void set_path(const std::string& p) {
    path = p;
  }

  void flush_data(size_t offset, size_t len) {
    // Do nothing
  }

  std::string path;
  monolog::monolog_linear_base<uint8_t, 65536, 1073741824> data_log_;
};

struct durable {
  durable() = default;

  void set_path(const std::string& p) {
    path = p;
    data_log_.init("data", path);
  }

  void flush_data(size_t offset, size_t len) {
    data_log_.flush(offset, len);
  }

  std::string path;
  monolog::mmap_monolog_base<uint8_t, 65536, 1073741824> data_log_;
};

struct durable_relaxed {
  durable_relaxed() = default;

  void set_path(const std::string& p) {
    path = p;
    data_log_.init("data", path);
  }

  void flush_ids(uint64_t start_id, size_t count) {
    // Do nothing
  }

  void flush_data(size_t offset, size_t len) {
    // Do nothing
  }

  std::string path;
  monolog::mmap_monolog_base<uint8_t, 65536, 1073741824> data_log_;
};

template<typename storage, typename concurrency_control,
    typename aux_data = empty_aux>
class dialog_store {
 public:
  typedef concurrency_control cc;

  dialog_store() = default;

  dialog_store(const std::string& path) {
    primary_.set_path(path);
  }

  uint64_t append(const uint8_t* data, size_t length) {
    uint64_t offset = cc_.start_write_op(length);
    primary_.data_log_.write(offset, data, length);
    primary_.flush_data(offset, length);
    aux_.update(offset, data, length);
    cc_.end_write_op(offset, length);
    return offset;
  }

  template<typename T>
  uint64_t append(const T& data) {
    uint64_t length = serializer<T>::size(data);
    uint64_t offset = cc_.start_write_op(length);
    serializer<T>::serialize(primary_.data_log_.ptr(offset), data);
    primary_.flush_data(offset, length);
    aux_.update(offset, data);
    cc_.end_write_op(offset, length);
    return offset;
  }

  uint64_t append_batch(const std::vector<uint8_t*>& batch,
                        std::vector<size_t>& lengths) {
    size_t length = std::accumulate(lengths.begin(), lengths.end(), 0);
    uint64_t offset = cc_.start_write_op(length);
    uint64_t off = offset;
    for (size_t i = 0; i < batch.size(); i++) {
      primary_.data_log_.write(off, batch.at(i), lengths.at(i));
      aux_.update(off, batch.at(i), lengths.at(i));
      off += lengths.at(i);
    }
    primary_.flush_data(offset, length);
    cc_.end_write_op(offset, length);
    return offset;
  }

  template<typename T>
  uint64_t append_batch(const std::vector<T>& batch) {
    size_t length = 0;
    std::vector<size_t> batch_offsets;
    for (size_t i = 0; i < batch.size(); i++) {
      batch_offsets.push_back(length);
      length += serializer<T>::size(batch.at(i));
    }
    uint64_t offset = cc_.start_write_op(length);
    for (size_t i = 0; i < batch.size(); i++) {
      serializer<T>::serialize(
          primary_.data_log_.ptr(offset + batch_offsets.at(i)), batch.at(i));
      aux_.update(offset + batch_offsets.at(i), batch.at(i));
    }
    primary_.flush_data(offset, length);
    cc_.end_write_op(offset, length);
    return offset;
  }

  bool read(uint64_t offset, uint8_t* data, size_t length,
            uint64_t tail) const {
    if (cc_.is_valid(offset, tail)) {
      primary_.data_log_.read(offset, data, length);
      return true;
    }
    return false;
  }

  template<typename T>
  bool read(uint64_t offset, T* data, uint64_t tail) const {
    if (cc_.is_valid(offset, tail)) {
      deserializer<T>::deserialize(primary_.data_log_.cptr(offset), data);
      return true;
    }
    return false;
  }

  bool get(uint64_t offset, uint8_t* data, size_t length) const {
    uint64_t tail = cc_.get_tail();
    return read(offset, data, length, tail);
  }

  template<typename T>
  bool get(uint64_t offset, T* data) const {
    uint64_t tail = cc_.get_tail();
    return read(offset, data, tail);
  }

  bool update(uint64_t id, const uint8_t* data, size_t length) {
    assert_throw(0, "Not supported");
    return false;
  }

  template<typename T>
  bool update(uint64_t id, const T& data) {
    assert_throw(0, "Not supported");
    return false;
  }

  bool invalidate(uint64_t id) {
    assert_throw(0, "Not supported");
    return false;
  }

  size_t num_records() const {
    return cc_.get_tail();
  }

 protected:
  storage primary_;
  aux_data aux_;
  concurrency_control cc_;
};

}

}

#endif /* DIALOG_REFLOG_H_ */

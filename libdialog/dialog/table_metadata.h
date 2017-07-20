#ifndef DIALOG_TABLE_METADATA_H_
#define DIALOG_TABLE_METADATA_H_

#include <cstdint>
#include <string>
#include <fstream>

namespace dialog {

enum metadata_type
  : uint32_t {
    D_INDEX_METADATA,
  D_FILTER_METADATA,
  D_TRIGGER_METADATA
};

struct __index_info {
 public:
  __index_info(uint32_t index_id, const std::string& name, double bucket_size)
      : index_id_(index_id),
        name_(name),
        bucket_size_(bucket_size) {
  }

  __index_info(const __index_info& other)
  : index_id_(other.index_id_),
  name_(other.name_),
  bucket_size_(other.bucket_size_) {
  }

  uint32_t index_id() const {
    return index_id_;
  }

  const std::string& name() const {
    return name_;
  }

  double bucket_size() const {
    return bucket_size_;
  }

private:
  uint32_t index_id_;
  const std::string name_;
  double bucket_size_;
};

struct __filter_info {
 public:
  __filter_info(uint32_t filter_id, const std::string& expr)
      : filter_id_(filter_id),
        expr_(expr) {
  }

  __filter_info(const __filter_info& other)
  : filter_id_(other.filter_id_),
  expr_(other.expr_) {
  }

  uint32_t filter_id() const {
    return filter_id_;
  }

  const std::string& expr() const {
    return expr_;
  }

private:
  uint32_t filter_id_;
  const std::string expr_;
};

struct __trigger_info {
 public:
  __trigger_info(uint32_t trigger_id, uint32_t filter_id, aggregate_id agg_id,
                 const std::string& name, relop_id op,
                 const mutable_value& threshold)
      : trigger_id_(trigger_id),
        filter_id_(filter_id),
        agg_id_(agg_id),
        name_(name),
        op_(op),
        threshold_(threshold) {
  }

  __trigger_info(const __trigger_info& other)
  : trigger_id_(other.trigger_id_),
  filter_id_(other.filter_id_),
  agg_id_(other.agg_id_),
  name_(other.name_),
  op_(other.op_),
  threshold_(other.threshold_) {
  }

  uint32_t trigger_id() const {
    return trigger_id_;
  }

  uint32_t filter_id() const {
    return filter_id_;
  }

  aggregate_id agg_id() const {
    return agg_id_;
  }

  relop_id op() const {
    return op_;
  }

  const std::string& name() const {
    return name_;
  }

  const mutable_value& threshold() const {
    return threshold_;
  }

private:
  uint32_t trigger_id_;
  uint32_t filter_id_;
  aggregate_id agg_id_;
  const std::string name_;
  relop_id op_;
  mutable_value threshold_;
};

template<class storage_mode>
class metadata_writer {
 public:
  metadata_writer(const std::string& path)
      : filename_(path + "/metadata"),
        out_(filename_) {
  }

  void write_index_info(uint32_t index_id, const std::string& name,
                        double bucket_size) {
    metadata_type type = metadata_type::D_INDEX_METADATA;
    out_.write(reinterpret_cast<const char*>(&type), sizeof(metadata_type));
    out_.write(reinterpret_cast<const char*>(&index_id), sizeof(uint32_t));
    size_t expr_size = name.length();
    out_.write(reinterpret_cast<const char*>(&expr_size), sizeof(size_t));
    out_.write(name.c_str(), expr_size);
    out_.write(reinterpret_cast<const char*>(&bucket_size), sizeof(double));
    out_.flush();
  }

  void write_filter_info(uint32_t filter_id, const std::string& expr) {
    metadata_type type = metadata_type::D_FILTER_METADATA;
    out_.write(reinterpret_cast<const char*>(&type), sizeof(metadata_type));
    out_.write(reinterpret_cast<const char*>(&filter_id), sizeof(uint32_t));
    size_t expr_size = expr.length();
    out_.write(reinterpret_cast<const char*>(&expr_size), sizeof(size_t));
    out_.write(expr.c_str(), expr_size);
    out_.flush();
  }

  void write_trigger_info(uint32_t trigger_id, uint32_t filter_id,
                          aggregate_id agg_id, const std::string& name,
                          relop_id op, const mutable_value& threshold) {
    metadata_type type = metadata_type::D_TRIGGER_METADATA;
    out_.write(reinterpret_cast<const char*>(&type), sizeof(metadata_type));
    out_.write(reinterpret_cast<const char*>(&trigger_id), sizeof(uint32_t));
    out_.write(reinterpret_cast<const char*>(&filter_id), sizeof(uint32_t));
    out_.write(reinterpret_cast<const char*>(&agg_id), sizeof(aggregate_id));
    size_t name_size = name.length();
    out_.write(reinterpret_cast<const char*>(&name_size), sizeof(size_t));
    out_.write(name.c_str(), name_size);
    out_.write(reinterpret_cast<const char*>(&op), sizeof(relop_id));
    type_id id = threshold.type().id;
    size_t type_size = threshold.type().size;
    out_.write(reinterpret_cast<const char*>(&id), sizeof(type_id));
    out_.write(reinterpret_cast<const char*>(threshold.ptr()), type_size);
    out_.flush();
  }

 private:
  std::string filename_;
  std::ofstream out_;
};

template<>
class metadata_writer<storage::in_memory> {
 public:
  metadata_writer(const std::string& path) {
  }

  void write_index_info(uint32_t index_id, const std::string& name,
                        double bucket_size) {
  }

  void write_filter_info(uint32_t filter_id, const std::string& expr) {
  }

  void write_trigger_info(uint32_t trigger_id, uint32_t filter_id,
                          aggregate_id agg_id, const std::string& name,
                          relop_id op, const mutable_value& threshold) {
  }
};

class metadata_reader {
 public:
  metadata_reader(const std::string& path)
      : filename_(path + "/metadata"),
        in_(filename_) {
  }

  metadata_type next_type() {
    metadata_type type;
    in_.read(reinterpret_cast<char*>(&type), sizeof(metadata_type));
    return type;
  }

  __index_info next_index_info() {
    uint32_t index_id;
    size_t name_size;
    double bucket_size;
    in_.read(reinterpret_cast<char*>(&index_id), sizeof(uint32_t));
    in_.read(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    in_.read(buf_, name_size);
    in_.read(reinterpret_cast<char*>(&bucket_size), sizeof(double));
    return __index_info(index_id, std::string(buf_, name_size), bucket_size);
  }

  __filter_info next_filter_info() {
    uint32_t filter_id;
    size_t expr_size;
    in_.read(reinterpret_cast<char*>(&filter_id), sizeof(uint32_t));
    in_.read(reinterpret_cast<char*>(&expr_size), sizeof(size_t));
    in_.read(buf_, expr_size);
    return __filter_info(filter_id, std::string(buf_, expr_size));
  }

  __trigger_info next_trigger_info() {
    uint32_t trigger_id;
    uint32_t filter_id;
    aggregate_id agg_id;
    size_t name_size;
    type_id tid;
    relop_id op;

    in_.read(reinterpret_cast<char*>(&trigger_id), sizeof(uint32_t));
    in_.read(reinterpret_cast<char*>(&filter_id), sizeof(uint32_t));
    in_.read(reinterpret_cast<char*>(&agg_id), sizeof(aggregate_id));
    in_.read(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    in_.read(buf_, name_size);
    in_.read(reinterpret_cast<char*>(&op), sizeof(relop_id));
    in_.read(reinterpret_cast<char*>(&tid), sizeof(type_id));
    data_type type(tid);
    uint8_t* buf = new uint8_t[type.size];
    in_.read(reinterpret_cast<char*>(buf), type.size);
    mutable_value threshold(type, buf);
    delete[] buf;
    return __trigger_info(trigger_id, filter_id, agg_id,
                          std::string(buf_, name_size), op, threshold);
  }

 private:
  std::string filename_;
  std::ifstream in_;
  char buf_[65536];
};

}

#endif /* DIALOG_TABLE_METADATA_H_ */

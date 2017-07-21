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

struct index_info {
 public:
  index_info(uint32_t index_id, const std::string& name, double bucket_size)
      : index_id_(index_id),
        name_(name),
        bucket_size_(bucket_size) {
  }

  index_info(const index_info& other)
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

struct filter_info {
 public:
  filter_info(uint32_t filter_id, const std::string& expr)
      : filter_id_(filter_id),
        expr_(expr) {
  }

  filter_info(const filter_info& other)
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

struct trigger_info {
 public:
  trigger_info(uint32_t trigger_id, uint32_t filter_id, aggregate_id agg_id,
               const std::string& name, relop_id op, const numeric& threshold)
      : trigger_id_(trigger_id),
        filter_id_(filter_id),
        agg_id_(agg_id),
        name_(name),
        op_(op),
        threshold_(threshold) {
  }

  trigger_info(const trigger_info& other)
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

  const numeric& threshold() const {
    return threshold_;
  }

 private:
  uint32_t trigger_id_;
  uint32_t filter_id_;
  aggregate_id agg_id_;
  const std::string name_;
  relop_id op_;
  numeric threshold_;
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
                          relop_id op, const numeric& threshold) {
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
    out_.write(reinterpret_cast<const char*>(&id), sizeof(type_id));
    switch (id) {
      case type_id::D_BOOL: {
        bool val = threshold.as<bool>();
        out_.write(reinterpret_cast<const char*>(&val), sizeof(bool));
        break;
      }
      case type_id::D_CHAR: {
        char val = threshold.as<char>();
        out_.write(reinterpret_cast<const char*>(&val), sizeof(char));
        break;
      }
      case type_id::D_SHORT: {
        short val = threshold.as<short>();
        out_.write(reinterpret_cast<const char*>(&val), sizeof(short));
        break;
      }
      case type_id::D_INT: {
        int val = threshold.as<int>();
        out_.write(reinterpret_cast<const char*>(&val), sizeof(int));
        break;
      }
      case type_id::D_LONG: {
        long val = threshold.as<long>();
        out_.write(reinterpret_cast<const char*>(&val), sizeof(long));
        break;
      }
      case type_id::D_FLOAT: {
        float val = threshold.as<float>();
        out_.write(reinterpret_cast<const char*>(&val), sizeof(float));
        break;
      }
      case type_id::D_DOUBLE: {
        double val = threshold.as<double>();
        out_.write(reinterpret_cast<const char*>(&val), sizeof(double));
        break;
      }
      default:
        THROW(invalid_operation_exception, "Threshold is not of numeric type");
    }
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
                          relop_id op, const numeric& threshold) {
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

  index_info next_index_info() {
    uint32_t index_id;
    size_t name_size;
    double bucket_size;
    in_.read(reinterpret_cast<char*>(&index_id), sizeof(uint32_t));
    in_.read(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    in_.read(buf_, name_size);
    in_.read(reinterpret_cast<char*>(&bucket_size), sizeof(double));
    return index_info(index_id, std::string(buf_, name_size), bucket_size);
  }

  filter_info next_filter_info() {
    uint32_t filter_id;
    size_t expr_size;
    in_.read(reinterpret_cast<char*>(&filter_id), sizeof(uint32_t));
    in_.read(reinterpret_cast<char*>(&expr_size), sizeof(size_t));
    in_.read(buf_, expr_size);
    return filter_info(filter_id, std::string(buf_, expr_size));
  }

  trigger_info next_trigger_info() {
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
    numeric threshold;
    switch (tid) {
      case type_id::D_BOOL: {
        bool val;
        in_.read(reinterpret_cast<char*>(&val), sizeof(bool));
        threshold = val;
        break;
      }
      case type_id::D_CHAR: {
        char val;
        in_.read(reinterpret_cast<char*>(&val), sizeof(char));
        threshold = val;
        break;
      }
      case type_id::D_SHORT: {
        short val;
        in_.read(reinterpret_cast<char*>(&val), sizeof(short));
        threshold = val;
        break;
      }
      case type_id::D_INT: {
        int val;
        in_.read(reinterpret_cast<char*>(&val), sizeof(int));
        threshold = val;
        break;
      }
      case type_id::D_LONG: {
        long val;
        in_.read(reinterpret_cast<char*>(&val), sizeof(long));
        threshold = val;
        break;
      }
      case type_id::D_FLOAT: {
        float val;
        in_.read(reinterpret_cast<char*>(&val), sizeof(float));
        threshold = val;
        break;
      }
      case type_id::D_DOUBLE: {
        double val;
        in_.read(reinterpret_cast<char*>(&val), sizeof(double));
        threshold = val;
        break;
      }
      default:
        THROW(invalid_operation_exception, "Threshold is not of numeric type");
    }
    return trigger_info(trigger_id, filter_id, agg_id,
                        std::string(buf_, name_size), op, threshold);
  }

 private:
  std::string filename_;
  std::ifstream in_;
  char buf_[65536];
};

}

#endif /* DIALOG_TABLE_METADATA_H_ */

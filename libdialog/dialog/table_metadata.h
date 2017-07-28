#ifndef DIALOG_TABLE_METADATA_H_
#define DIALOG_TABLE_METADATA_H_

#include <cstdint>
#include <string>
#include <fstream>

#include "storage.h"
#include "io_utils.h"

using namespace utils;

namespace dialog {

enum metadata_type
  : uint32_t {
    D_INDEX_METADATA,
  D_FILTER_METADATA,
  D_TRIGGER_METADATA
};

struct index_info {
 public:
  index_info(const std::string& field_name, double bucket_size)
      : field_name_(field_name),
        bucket_size_(bucket_size) {
  }

  std::string field_name() const {
    return field_name_;
  }

  double bucket_size() const {
    return bucket_size_;
  }

 private:
  std::string field_name_;
  double bucket_size_;
};

struct filter_info {
 public:
  filter_info(const std::string& filter_name, const std::string& expr)
      : filter_name_(filter_name),
        expr_(expr) {
  }

  filter_info(const filter_info& other)
      : filter_name_(other.filter_name_),
        expr_(other.expr_) {
  }

  const std::string& filter_name() const {
    return filter_name_;
  }

  const std::string& expr() const {
    return expr_;
  }

 private:
  std::string filter_name_;
  std::string expr_;
};

struct trigger_info {
 public:
  trigger_info(const std::string& trigger_name, const std::string& filter_name,
               aggregate_id agg_id, const std::string& field_name, relop_id op,
               const numeric& threshold)
      : trigger_id_(trigger_name),
        filter_id_(filter_name),
        agg_id_(agg_id),
        field_name_(field_name),
        op_(op),
        threshold_(threshold) {
  }

  const std::string& trigger_name() const {
    return trigger_id_;
  }

  const std::string& filter_name() const {
    return filter_id_;
  }

  aggregate_id agg_id() const {
    return agg_id_;
  }

  relop_id op() const {
    return op_;
  }

  const std::string& field_name() const {
    return field_name_;
  }

  const numeric& threshold() const {
    return threshold_;
  }

 private:
  std::string trigger_id_;
  std::string filter_id_;
  aggregate_id agg_id_;
  std::string field_name_;
  relop_id op_;
  numeric threshold_;
};

class metadata_writer {
 public:
  metadata_writer(const std::string& path, storage::storage_id id)
      : filename_(path + "/metadata"),
        id_(id) {
    if (id_ != storage::storage_id::D_IN_MEMORY) {
      out_.open(filename_);
    }
  }

  void write_index_info(const std::string& name, double bucket_size) {
    if (id_ != storage::storage_id::D_IN_MEMORY) {
      metadata_type type = metadata_type::D_INDEX_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, name);
      io_utils::write(out_, bucket_size);
      io_utils::flush(out_);
    }
  }

  void write_filter_info(const std::string& filter_name,
                         const std::string& expr) {
    if (id_ != storage::storage_id::D_IN_MEMORY) {
      metadata_type type = metadata_type::D_FILTER_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, filter_name);
      io_utils::write(out_, expr);
      io_utils::flush(out_);
    }
  }

  void write_trigger_info(const std::string& trigger_name,
                          const std::string& filter_name, aggregate_id agg_id,
                          const std::string& field_name, relop_id op,
                          const numeric& threshold) {
    if (id_ != storage::storage_id::D_IN_MEMORY) {
      metadata_type type = metadata_type::D_TRIGGER_METADATA;
      io_utils::write(out_, type);
      io_utils::write(out_, trigger_name);
      io_utils::write(out_, filter_name);
      io_utils::write(out_, agg_id);
      io_utils::write(out_, field_name);
      io_utils::write(out_, op);
      type_id id = threshold.type().id;
      io_utils::write(out_, id);
      switch (id) {
        case type_id::D_BOOL: {
          io_utils::write(out_, threshold.as<bool>());
          break;
        }
        case type_id::D_CHAR: {
          io_utils::write(out_, threshold.as<int8_t>());
          break;
        }
        case type_id::D_SHORT: {
          io_utils::write(out_, threshold.as<int16_t>());
          break;
        }
        case type_id::D_INT: {
          io_utils::write(out_, threshold.as<int32_t>());
          break;
        }
        case type_id::D_LONG: {
          io_utils::write(out_, threshold.as<int64_t>());
          break;
        }
        case type_id::D_FLOAT: {
          io_utils::write(out_, threshold.as<float>());
          break;
        }
        case type_id::D_DOUBLE: {
          io_utils::write(out_, threshold.as<double>());
          break;
        }
        default:
          THROW(invalid_operation_exception,
                "Threshold is not of numeric type");
      }
      io_utils::flush(out_);
    }
  }

 private:
  std::string filename_;
  std::ofstream out_;
  storage::storage_id id_;
};

class metadata_reader {
 public:
  metadata_reader(const std::string& path)
      : filename_(path + "/metadata"),
        in_(filename_) {
  }

  metadata_type next_type() {
    return io_utils::read<metadata_type>(in_);
  }

  index_info next_index_info() {
    std::string field_name = io_utils::read<std::string>(in_);
    double bucket_size = io_utils::read<double>(in_);
    return index_info(field_name, bucket_size);
  }

  filter_info next_filter_info() {
    std::string filter_name = io_utils::read<std::string>(in_);
    std::string expr = io_utils::read<std::string>(in_);
    return filter_info(filter_name, expr);
  }

  trigger_info next_trigger_info() {
    std::string trigger_name = io_utils::read<std::string>(in_);
    std::string filter_name = io_utils::read<std::string>(in_);
    aggregate_id agg_id = io_utils::read<aggregate_id>(in_);
    std::string field_name = io_utils::read<std::string>(in_);
    relop_id op = io_utils::read<relop_id>(in_);
    type_id tid = io_utils::read<type_id>(in_);
    numeric threshold;
    switch (tid) {
      case type_id::D_BOOL: {
        threshold = io_utils::read<bool>(in_);
        break;
      }
      case type_id::D_CHAR: {
        threshold = io_utils::read<int8_t>(in_);
        break;
      }
      case type_id::D_SHORT: {
        threshold = io_utils::read<int16_t>(in_);
        break;
      }
      case type_id::D_INT: {
        threshold = io_utils::read<int32_t>(in_);
        break;
      }
      case type_id::D_LONG: {
        threshold = io_utils::read<int64_t>(in_);
        break;
      }
      case type_id::D_FLOAT: {
        threshold = io_utils::read<float>(in_);
        break;
      }
      case type_id::D_DOUBLE: {
        threshold = io_utils::read<double>(in_);
        break;
      }
      default:
        THROW(invalid_operation_exception, "Threshold is not of numeric type");
    }
    return trigger_info(trigger_name, filter_name, agg_id, field_name, op,
                        threshold);
  }

 private:
  std::string filename_;
  std::ifstream in_;
};

}

#endif /* DIALOG_TABLE_METADATA_H_ */

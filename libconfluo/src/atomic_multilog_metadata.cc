#include "atomic_multilog_metadata.h"

namespace confluo {

index_metadata::index_metadata(const std::string &field_name, double bucket_size)
    : field_name_(field_name),
      bucket_size_(bucket_size) {
}
std::string index_metadata::field_name() const {
  return field_name_;
}
double index_metadata::bucket_size() const {
  return bucket_size_;
}
filter_metadata::filter_metadata(const std::string &filter_name, const std::string &expr)
    : filter_name_(filter_name),
      expr_(expr) {
}
const std::string &filter_metadata::filter_name() const {
  return filter_name_;
}
const std::string &filter_metadata::expr() const {
  return expr_;
}
aggregate_metadata::aggregate_metadata(const std::string &name, const std::string &filter_name, const std::string &expr)
    : name_(name),
      filter_name_(filter_name),
      expr_(expr) {
}
const std::string &aggregate_metadata::aggregate_name() const {
  return name_;
}
const std::string &aggregate_metadata::filter_name() const {
  return filter_name_;
}
const std::string &aggregate_metadata::aggregate_expression() const {
  return expr_;
}
trigger_metadata::trigger_metadata(const std::string &name, const std::string &expr, uint64_t periodicity_ms)
    : name_(name),
      expr_(expr),
      periodicity_ms_(periodicity_ms) {
}
const std::string &trigger_metadata::trigger_name() const {
  return name_;
}
const std::string &trigger_metadata::trigger_expression() const {
  return expr_;
}
uint64_t trigger_metadata::periodicity_ms() const {
  return periodicity_ms_;
}
metadata_writer::metadata_writer()
    : metadata_writer("") {
  state_ = UNINIT;
}
metadata_writer::metadata_writer(const std::string &path)
    : filename_(path + "/metadata"),
      state_(INIT) {
  out_.open(filename_);
}
metadata_writer::metadata_writer(const metadata_writer &other)
    : filename_(other.filename_),
      state_(other.state_) {
  out_.close();
  out_.open(filename_);
}
metadata_writer::~metadata_writer() {
  out_.close();
}
metadata_writer &metadata_writer::operator=(const metadata_writer &other) {
  out_.close();
  filename_ = other.filename_;
  out_.open(filename_);
  state_ = other.state_;
  return *this;
}
void metadata_writer::write_storage_mode(const storage::storage_mode mode) {
  if (state_) {
    metadata_type type = metadata_type::D_STORAGE_MODE_METADATA;
    io_utils::write(out_, type);
    io_utils::write(out_, mode);
    io_utils::flush(out_);
  }
}
void metadata_writer::write_archival_mode(const archival::archival_mode mode) {
  if (state_) {
    metadata_type type = metadata_type::D_ARCHIVAL_MODE_METADATA;
    io_utils::write(out_, type);
    io_utils::write(out_, mode);
    io_utils::flush(out_);
  }
}
void metadata_writer::write_schema(const schema_t &schema) {
  if (state_) {
    metadata_type type = metadata_type::D_SCHEMA_METADATA;
    io_utils::write(out_, type);
    io_utils::write(out_, schema.columns().size());
    for (auto& col : schema.columns()) {
      io_utils::write(out_, col.name());
      col.type().serialize(out_);
    }
    io_utils::flush(out_);
  }
}
void metadata_writer::write_index_metadata(const std::string &name, double bucket_size) {
  if (state_) {
    metadata_type type = metadata_type::D_INDEX_METADATA;
    io_utils::write(out_, type);
    io_utils::write(out_, name);
    io_utils::write(out_, bucket_size);
    io_utils::flush(out_);
  }
}
void metadata_writer::write_filter_metadata(const std::string &name, const std::string &expr) {
  if (state_) {
    metadata_type type = metadata_type::D_FILTER_METADATA;
    io_utils::write(out_, type);
    io_utils::write(out_, name);
    io_utils::write(out_, expr);
    io_utils::flush(out_);
  }
}
void metadata_writer::write_aggregate_metadata(const std::string &name,
                                               const std::string &filter_name,
                                               const std::string &expr) {
  if (state_) {
    metadata_type type = metadata_type::D_AGGREGATE_METADATA;
    io_utils::write(out_, type);
    io_utils::write(out_, name);
    io_utils::write(out_, filter_name);
    io_utils::write(out_, expr);
    io_utils::flush(out_);
  }
}
void metadata_writer::write_trigger_metadata(const std::string &trigger_name,
                                             const std::string &trigger_expr,
                                             const uint64_t periodicity_ms) {
  if (filename_ != "/metadata") {
    metadata_type type = metadata_type::D_TRIGGER_METADATA;
    io_utils::write(out_, type);
    io_utils::write(out_, trigger_name);
    io_utils::write(out_, trigger_expr);
    io_utils::write(out_, periodicity_ms);
    io_utils::flush(out_);
  }
}
metadata_reader::metadata_reader(const std::string &path)
    : filename_(path + "/metadata"),
      in_(filename_) {
}
bool metadata_reader::has_next() {
  return !in_.eof();
}
metadata_type metadata_reader::next_type() {
  return io_utils::read<metadata_type>(in_);
}
schema_t metadata_reader::next_schema() {
  size_t ncolumns = io_utils::read<size_t>(in_);
  schema_builder builder;
  for (size_t i = 0; i < ncolumns; i++) {
    std::string name = io_utils::read<std::string>(in_);
    data_type type = data_type::deserialize(in_);
    builder.add_column(type, name);
  }
  return schema_t(builder.get_columns());
}
index_metadata metadata_reader::next_index_metadata() {
  std::string field_name = io_utils::read<std::string>(in_);
  double bucket_size = io_utils::read<double>(in_);
  return index_metadata(field_name, bucket_size);
}
filter_metadata metadata_reader::next_filter_metadata() {
  std::string filter_name = io_utils::read<std::string>(in_);
  std::string expr = io_utils::read<std::string>(in_);
  return filter_metadata(filter_name, expr);
}
aggregate_metadata metadata_reader::next_aggregate_metadata() {
  std::string name = io_utils::read<std::string>(in_);
  std::string filter_name = io_utils::read<std::string>(in_);
  std::string expr = io_utils::read<std::string>(in_);
  return aggregate_metadata(name, filter_name, expr);
}
trigger_metadata metadata_reader::next_trigger_metadata() {
  std::string trigger_name = io_utils::read<std::string>(in_);
  std::string trigger_expr = io_utils::read<std::string>(in_);
  uint64_t periodicity_ms = io_utils::read<uint64_t>(in_);
  return trigger_metadata(trigger_name, trigger_expr, periodicity_ms);
}
storage::storage_mode metadata_reader::next_storage_mode() {
  return io_utils::read<storage::storage_mode>(in_);
}
archival::archival_mode metadata_reader::next_archival_mode() {
  return io_utils::read<archival::archival_mode>(in_);
}
}
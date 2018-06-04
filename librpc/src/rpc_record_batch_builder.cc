#include "rpc_record_batch_builder.h"

namespace confluo {
namespace rpc {

record_data::record_data(const void *data, size_t size)
    : std::string(reinterpret_cast<const char *>(data), size) {
}
record_data::record_data()
    : std::string() {
}
rpc_record_batch_builder::rpc_record_batch_builder(const schema_t &schema)
    : nrecords_(0),
      schema_(schema) {
}
void rpc_record_batch_builder::add_record(const record_data &rec) {
  int64_t ts = *reinterpret_cast<const int64_t *>(rec.data());
  int64_t time_block = ts / configuration_params::TIME_RESOLUTION_NS();
  batch_sizes_[time_block] += schema_.record_size();
  batch_[time_block].write(rec.data(), schema_.record_size());
  nrecords_++;
}
void rpc_record_batch_builder::add_record(const std::vector<std::string> &rec) {
  record_data rdata;
  schema_.record_vector_to_data(rdata, rec);
  add_record(rdata);
}
rpc_record_batch rpc_record_batch_builder::get_batch() {
  rpc_record_batch batch;
  batch.blocks.resize(batch_.size());
  batch.nrecords = static_cast<int64_t>(nrecords_);
  size_t i = 0;
  for (auto &entry : batch_) {
    batch.blocks[i].time_block = entry.first;
    batch.blocks[i].data = entry.second.str();
    batch.blocks[i].nrecords = static_cast<int64_t>(batch.blocks[i].data.size() / schema_.record_size());
    i++;
  }
  clear();
  return batch;
}
void rpc_record_batch_builder::clear() {
  batch_sizes_.clear();
  batch_.clear();
  nrecords_ = 0;
}
size_t rpc_record_batch_builder::num_records() const {
  return nrecords_;
}
}
}
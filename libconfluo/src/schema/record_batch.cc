#include "schema/record_batch.h"

namespace confluo {

const int64_t record_batch_builder::TIME_BLOCK;

int64_t record_batch::start_time_block() const {
  return blocks.front().time_block;
}

int64_t record_batch::end_time_block() const {
  return blocks.back().time_block;
}

record_batch_builder::record_batch_builder(const schema_t &schema)
    : schema_(schema) {
}

void record_batch_builder::add_record(const void *data) {
  size_t record_size = schema_.record_size();
  int64_t ts = *reinterpret_cast<const int64_t *>(data);
  int64_t time_block = ts / TIME_BLOCK;
  batch_sizes_[time_block] += record_size;
  batch_[time_block].write(reinterpret_cast<const char *>(data), record_size);
}

void record_batch_builder::add_record(const std::vector<std::string> &rec) {
  void *data = schema_.record_vector_to_data(rec);
  add_record(data);
  delete[] reinterpret_cast<uint8_t *>(data);
}

record_batch record_batch_builder::get_batch() {
  record_batch batch;
  batch.blocks.resize(batch_.size());
  batch.nrecords = 0;
  size_t i = 0;
  for (auto &entry : batch_) {
    batch.blocks[i].time_block = entry.first;
    batch.blocks[i].data = entry.second.str();
    batch.blocks[i].nrecords = batch.blocks[i].data.size()
        / schema_.record_size();
    batch.nrecords += batch.blocks[i].nrecords;
    i++;
  }
  return batch;
}

}
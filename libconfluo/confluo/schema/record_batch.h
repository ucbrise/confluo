#ifndef CONFLUO_SCHEMA_RECORD_BATCH_H_
#define CONFLUO_SCHEMA_RECORD_BATCH_H_

#include <map>
#include <vector>
#include <sstream>

#include "schema/schema.h"

namespace confluo {

struct record_block {
  int64_t time_block;
  std::string data;
  size_t nrecords;
};

struct record_batch {
  std::vector<record_block> blocks;
  size_t nrecords;

  int64_t start_time_block() const {
    return blocks.front().time_block;
  }

  int64_t end_time_block() const {
    return blocks.back().time_block;
  }
};

class record_batch_builder {
 public:
  static const int64_t TIME_BLOCK = 1e6;

  /**
   * Constructor for record batch builder
   * @param schema Schema for the atomic multilog
   */
  record_batch_builder(const schema_t& schema)
      : schema_(schema) {

  }

  /**
   * Adds record data to the batch
   * @param rec The record data to be added
   */
  void add_record(const void* data) {
    size_t record_size = schema_.record_size();
    int64_t ts = *reinterpret_cast<const int64_t*>(data);
    int64_t time_block = ts / TIME_BLOCK;
    batch_sizes_[time_block] += record_size;
    batch_[time_block].write(reinterpret_cast<const char*>(data), record_size);
  }

  /**
   * Adds record to the batch
   * @param rec The record to be added
   */
  void add_record(const std::vector<std::string>& rec) {
    void* data = schema_.record_vector_to_data(rec);
    add_record(data);
    delete[] reinterpret_cast<uint8_t*>(data);
  }

  /**
   * Gets the batch of records
   * @return The batch
   */
  record_batch get_batch() {
    record_batch batch;
    batch.blocks.resize(batch_.size());
    batch.nrecords = 0;
    size_t i = 0;
    for (auto& entry : batch_) {
      batch.blocks[i].time_block = entry.first;
      batch.blocks[i].data = entry.second.str();
      batch.blocks[i].nrecords = batch.blocks[i].data.size()
          / schema_.record_size();
      batch.nrecords += batch.blocks[i].nrecords;
      i++;
    }
    return batch;
  }

 private:
  std::map<int64_t, size_t> batch_sizes_;
  std::map<int64_t, std::stringstream> batch_;
  const schema_t& schema_;
};

const int64_t record_batch_builder::TIME_BLOCK;

}

#endif /* CONFLUO_SCHEMA_RECORD_BATCH_H_ */

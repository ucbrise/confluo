#ifndef CONFLUO_SCHEMA_RECORD_BATCH_H_
#define CONFLUO_SCHEMA_RECORD_BATCH_H_

#include <map>
#include <vector>

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

  record_batch_builder() = default;

  /**
   * Adds record to the batch
   * @param rec The record to be added
   */
  void add_record(const std::string& rec) {
    int64_t ts = *reinterpret_cast<const int64_t*>(rec.data());
    int64_t time_block = ts / TIME_BLOCK;
    batch_sizes_[time_block] += rec.size();
    batch_[time_block].push_back(rec);
  }

  /**
   * Moves a record to the batch
   * @param rec The record to be moved
   */
  void add_record(std::string&& rec) {
    int64_t ts = *reinterpret_cast<const int64_t*>(rec.data());
    int64_t time_block = ts / TIME_BLOCK;
    batch_sizes_[time_block] += rec.size();
    batch_[time_block].push_back(std::move(rec));
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
    for (auto entry : batch_) {
      batch.blocks[i].time_block = entry.first;
      batch.blocks[i].nrecords = entry.second.size();
      batch.blocks[i].data.resize(batch_sizes_[entry.first]);
      char* buf = &(batch.blocks[i].data[0]);
      size_t accum = 0;
      for (size_t j = 0; j < entry.second.size(); j++) {
        void* dst = buf + accum;
        memcpy(dst, entry.second[j].data(), entry.second[j].size());
        accum += entry.second[j].size();
        batch.nrecords++;
      }
      i++;
    }
    return batch;
  }

 private:
  std::map<int64_t, size_t> batch_sizes_;
  std::map<int64_t, std::vector<std::string>> batch_;
};

const int64_t record_batch_builder::TIME_BLOCK;

}

#endif /* CONFLUO_SCHEMA_RECORD_BATCH_H_ */

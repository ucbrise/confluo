#ifndef RPC_RPC_RECORD_BATCH_BUILDER_H_
#define RPC_RPC_RECORD_BATCH_BUILDER_H_

#include <map>
#include <vector>

#include "configuration_params.h"

namespace confluo {
namespace rpc {

class rpc_record_batch_builder {
 public:
  rpc_record_batch_builder()
      : nrecords_(0) {
  }

  void add_record(const std::string& rec) {
    int64_t ts = *reinterpret_cast<const int64_t*>(rec.data());
    int64_t time_block = ts / configuration_params::TIME_RESOLUTION_NS;
    batch_sizes_[time_block] += rec.size();
    batch_[time_block].push_back(rec);
    nrecords_++;
  }

  void add_record(std::string&& rec) {
    int64_t ts = *reinterpret_cast<const int64_t*>(rec.data());
    int64_t time_block = ts / configuration_params::TIME_RESOLUTION_NS;
    batch_sizes_[time_block] += rec.size();
    batch_[time_block].push_back(std::move(rec));
  }

  rpc_record_batch get_batch() {
    rpc_record_batch batch;
    batch.blocks.resize(batch_.size());
    batch.nrecords = nrecords_;
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
      }
      i++;
    }
    clear();
    return batch;
  }

  void clear() {
    batch_sizes_.clear();
    batch_.clear();
    nrecords_ = 0;
  }

  size_t num_records() const {
    return nrecords_;
  }

 private:
  size_t nrecords_;
  std::map<int64_t, size_t> batch_sizes_;
  std::map<int64_t, std::vector<std::string>> batch_;
};

}
}

#endif /* RPC_RPC_RECORD_BATCH_BUILDER_H_ */

#ifndef RPC_RPC_RECORD_BATCH_BUILDER_H_
#define RPC_RPC_RECORD_BATCH_BUILDER_H_

#include <map>
#include <vector>

#include "conf/configuration_params.h"

namespace confluo {
namespace rpc {

struct record_data : public std::string {
  record_data(const void* data, size_t size)
      : std::string(reinterpret_cast<const char*>(data), size) {
  }

  record_data()
      : std::string() {
  }
};

class rpc_record_batch_builder {
 public:
  rpc_record_batch_builder(const schema_t& schema)
      : nrecords_(0),
        schema_(schema) {
  }

  void add_record(const record_data& rec) {
    int64_t ts = *reinterpret_cast<const int64_t*>(rec.data());
    int64_t time_block = ts / configuration_params::TIME_RESOLUTION_NS;
    batch_sizes_[time_block] += schema_.record_size();
    batch_[time_block].write(rec.data(), schema_.record_size());
    nrecords_++;
  }

  void add_record(const std::vector<std::string>& rec) {
    record_data rdata;
    schema_.record_vector_to_data(rdata, rec);
    add_record(rdata);
  }

  rpc_record_batch get_batch() {
    rpc_record_batch batch;
    batch.blocks.resize(batch_.size());
    batch.nrecords = nrecords_;
    size_t i = 0;
    for (auto& entry : batch_) {
      batch.blocks[i].time_block = entry.first;
      batch.blocks[i].data = entry.second.str();
      batch.blocks[i].nrecords = batch.blocks[i].data.size()
          / schema_.record_size();
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
  std::map<int64_t, std::stringstream> batch_;
  const schema_t& schema_;
};

}
}

#endif /* RPC_RPC_RECORD_BATCH_BUILDER_H_ */

#include "rpc_type_conversions.h"

namespace confluo {
namespace rpc {

record_batch rpc_type_conversions::convert_batch(const rpc_record_batch &rpc_batch) {
  record_batch batch;
  batch.nrecords = static_cast<size_t>(rpc_batch.nrecords);
  batch.blocks.resize(rpc_batch.blocks.size());
  size_t i = 0;
  for (const rpc_record_block& rpc_block : rpc_batch.blocks) {
    batch.blocks[i].time_block = rpc_block.time_block;
    batch.blocks[i].nrecords = static_cast<size_t>(rpc_block.nrecords);
    batch.blocks[i].data = std::move(rpc_block.data);
    i++;
  }
  return batch;
}

std::vector<column_t> rpc_type_conversions::convert_schema(const rpc_schema &s) {
  schema_builder builder;
  for (const rpc_column& col : s) {
    data_type type(static_cast<uint16_t>(static_cast<size_t>(col.type_id)), static_cast<size_t>(col.type_size));
    builder.add_column(type, col.name);
  }
  return builder.get_columns();
}

rpc_schema rpc_type_conversions::convert_schema(const std::vector<column_t> &s) {
  rpc_schema schema;
  for (const column_t& col : s) {
    rpc_column c;
    c.name = col.name();
    c.type_id = static_cast<int32_t>(col.type().id);
    c.type_size = static_cast<int32_t>(col.type().size);
    schema.push_back(c);
  }
  return schema;
}

storage::storage_mode rpc_type_conversions::convert_mode(const rpc_storage_mode &mode) {
  return static_cast<storage::storage_mode>(mode);
}

rpc_storage_mode rpc_type_conversions::convert_mode(const storage::storage_mode &mode) {
  return static_cast<rpc_storage_mode>(mode);
}

}
}
#ifndef RPC_RPC_TYPE_CONVERSIONS_H_
#define RPC_RPC_TYPE_CONVERSIONS_H_

#include "schema/record_batch.h"
#include "schema/schema.h"
#include "storage/storage.h"
#include "rpc_types.h"


namespace confluo {
namespace rpc {
class rpc_type_conversions {
 public:
  static record_batch convert_batch(const rpc_record_batch& rpc_batch) {
    record_batch batch;
    batch.nrecords = rpc_batch.nrecords;
    batch.blocks.resize(rpc_batch.blocks.size());
    size_t i = 0;
    for (const rpc_record_block& rpc_block : rpc_batch.blocks) {
      batch.blocks[i].time_block = rpc_block.time_block;
      batch.blocks[i].nrecords = rpc_block.nrecords;
      batch.blocks[i].data = std::move(rpc_block.data);
      i++;
    }
    return batch;
  }

  static std::vector<column_t> convert_schema(const rpc_schema& s) {
    schema_builder builder;
    for (const rpc_column& col : s) {
      data_type type(static_cast<size_t>(col.type_id), col.type_size);
      builder.add_column(type, col.name);
    }
    return builder.get_columns();
  }

  static rpc_schema convert_schema(const std::vector<column_t>& s) {
    rpc_schema schema;
    for (const column_t& col : s) {
      rpc_column c;
      c.name = col.name();
      c.type_id = col.type().id;
      c.type_size = col.type().size;
      schema.push_back(c);
    }
    return schema;
  }

  static storage::storage_mode convert_mode(const rpc_storage_mode& mode) {
    return static_cast<storage::storage_mode>(mode);
  }

  static rpc_storage_mode convert_mode(const storage::storage_mode& mode) {
    return static_cast<rpc_storage_mode>(mode);
  }
};
}
}

#endif /* RPC_RPC_TYPE_CONVERSIONS_H_ */

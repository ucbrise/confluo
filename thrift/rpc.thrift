namespace cpp confluo.rpc
namespace java confluo.rpc
namespace py confluo.rpc

enum rpc_storage_mode {
  RPC_IN_MEMORY = 0,
  RPC_DURABLE_RELAXED = 1,
  RPC_DURABLE = 2
}

enum rpc_data_type {
  RPC_NONE = 0,
  RPC_BOOL = 1,
  RPC_CHAR = 2,
  RPC_UCHAR = 3,
  RPC_SHORT = 4,
  RPC_USHORT = 5,
  RPC_INT = 6,
  RPC_UINT = 7,
  RPC_LONG = 8,
  RPC_ULONG = 9,
  RPC_FLOAT = 10,
  RPC_DOUBLE = 11,
  RPC_STRING = 12,
  RPC_RECORD = 10001,
  RPC_ALERT = 10002
}

enum rpc_iterator_type {
  RPC_ADHOC = 0,
  RPC_PREDEF = 1,
  RPC_COMBINED = 2,
  RPC_ALERTS = 3,
}

struct rpc_column {
  1: required i32 type_id,
  2: required i32 type_size,
  3: required string name,
}

typedef list<rpc_column> rpc_schema;

typedef i64 rpc_iterator_id;
typedef i32 rpc_handler_id;

struct rpc_iterator_descriptor {
  1: required rpc_iterator_id id,
  2: required rpc_iterator_type type,
  3: required rpc_data_type data_type,
  4: required rpc_handler_id handler_id
}

struct rpc_iterator_handle {
  1: required rpc_iterator_descriptor desc,
  2: required binary data,
  3: required i32 num_entries,
  4: required bool has_more,
}

struct rpc_record_block {
  1: required i64 time_block,
  2: required binary data,
  3: required i64 nrecords,
}

struct rpc_record_batch {
  1: required list<rpc_record_block> blocks,
  2: required i64 nrecords,
}

struct rpc_atomic_multilog_info {
  1: i64 id,
  2: rpc_schema schema,
}

exception rpc_management_exception {
  1: string msg 
}

exception rpc_invalid_operation {
  1: string msg
}

service rpc_service {
  /** Management ops **/
  // Thread ops
  void register_handler() 
          throws (1: rpc_management_exception ex),
  void deregister_handler() 
          throws (1: rpc_management_exception ex),
  
  // Store ops
  i64 create_atomic_multilog(1: string name, 2: rpc_schema schema, 
                             3: rpc_storage_mode mode) 
          throws (1: rpc_management_exception ex),
  rpc_atomic_multilog_info get_atomic_multilog_info(1: string name),
  void remove_atomic_multilog(1: i64 multilog_id)
          throws (1: rpc_management_exception ex),
  
  // Table ops
  void add_index(1: i64 multilog_id, 2: string field_name, 
                 3: double bucket_size) 
          throws (1: rpc_management_exception ex),
  void remove_index(1: i64 multilog_id, 2: string field_name) 
          throws (1: rpc_management_exception ex),
  void add_filter(1: i64 multilog_id, 2: string filter_name, 
                  3: string filter_expr)
          throws (1: rpc_management_exception ex),
  void remove_filter(1: i64 multilog_id, 2: string filter_name) 
          throws (1: rpc_management_exception ex),
  void add_aggregate(1: i64 mutlilog_id, 2: string aggregate_name, 
                     3: string filter_name, 4: string aggregate_expr) 
          throws (1: rpc_management_exception ex),
  void remove_aggregate(1: i64 multilog_id, 2: string aggregate_name),
  void add_trigger(1: i64 multilog_id, 2: string trigger_name, 
                   3: string trigger_expr)
          throws (1: rpc_management_exception ex),
  void remove_trigger(1: i64 multilog_id, 2: string trigger_name) 
          throws (1: rpc_management_exception ex),
            
  /** Query ops **/
  // Write ops
  i64 append(1: i64 multilog_id, 2: binary data),
  i64 append_json(1: i64 multilog_id, 2: string data),
  i64 append_batch(1: i64 multilog_id, 2: rpc_record_batch batch),
  
  // Read ops
  binary read(1: i64 multilog_id, 2: i64 offset, 3: i64 nrecords),
  string read_json(1: i64 multilog_id, 2: i64 offset, 3: i64 nrecords),
  string query_aggregate(1: i64 multilog_id, 2: string aggregate_name, 
                         3: i64 begin_ms, 4: i64 end_ms)
          throws (1: rpc_invalid_operation ex),
  string adhoc_aggregate(1: i64 multilog_id, 2: string aggregate_expr, 
                         3: string filter_expr)
          throws (1: rpc_invalid_operation ex),
  rpc_iterator_handle adhoc_filter(1: i64 multilog_id, 2: string filter_expr)
          throws (1: rpc_invalid_operation ex),
  rpc_iterator_handle predef_filter(1: i64 multilog_id, 2: string filter_name, 
                                    3: i64 begin_ms, 4: i64 end_ms)
          throws (1: rpc_invalid_operation ex),
  rpc_iterator_handle combined_filter(1: i64 multilog_id, 2: string filter_name,
                                      3: string filter_expr, 
                                      4: i64 begin_ms, 5: i64 end_ms)
          throws (1: rpc_invalid_operation ex),
  rpc_iterator_handle alerts_by_time(1: i64 multilog_id, 2: i64 begin_ms, 
                                     3: i64 end_ms)
          throws (1: rpc_invalid_operation ex),
  rpc_iterator_handle alerts_by_trigger_and_time(1: i64 multilog_id,
                                                 2: string trigger_name,
                                                 3: i64 begin_ms, 4: i64 end_ms)
          throws (1: rpc_invalid_operation ex),
  rpc_iterator_handle get_more(1: i64 multilog_id, 
                               2: rpc_iterator_descriptor desc)
          throws (1: rpc_invalid_operation ex),

  i64 num_records(1: i64 multilog_id),  
}

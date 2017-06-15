namespace cpp dialog

service log_store_service {
  i64 append(1: binary data),
  list<i64> multi_append(1: list<binary> data),
  binary get(1: i64 id, 2: i64 len),
  bool update(1: i64 id, 2: binary data),
  bool invalidate(1: i64 id),
  i64 num_records(),
}

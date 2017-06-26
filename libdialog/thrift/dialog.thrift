namespace cpp dialog

service dialog_service {
  i64 append(1: binary data),
  list<i64> multi_append(1: list<binary> data),
  binary get(1: i64 id, 2: i64 len),
  i64 num_records(),
}

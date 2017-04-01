namespace java edu.berkeley.cs.timeseries
namespace cpp timeseries
namespace py timeseries

typedef i64 timestamp_t
typedef i64 version_t
typedef i64 value_t

service timeseries_db_service {
  version_t insert_values(1: binary pts),
  version_t insert_values_block(1: binary pts, 2: timestamp_t ts_block),
  binary get_range(1: timestamp_t start_ts, 2: timestamp_t end_ts, 3: version_t version),
  binary get_range_latest(1: timestamp_t start_ts, 2: timestamp_t end_ts),
  binary get_nearest_value(1: bool direction, 2: timestamp_t ts, 3: version_t version),
  binary get_nearest_value_latest(1: bool direction, 2: timestamp_t ts),
  binary compute_diff(1: version_t from_version, 2: version_t to_version),
  i64 num_entries(),
}

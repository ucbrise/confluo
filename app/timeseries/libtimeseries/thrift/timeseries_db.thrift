namespace java edu.berkeley.cs.timeseries
namespace cpp timeseries
namespace py timeseries

typedef i64 timestamp_t
typedef i64 version_t
typedef i64 value_t
typedef i64 uuid_t

service timeseries_db_service {
  void add_stream(1: uuid_t uuid),
  version_t insert_values(1: uuid_t uuid, 2: binary pts),
  version_t insert_values_block(1: uuid_t uuid, 2: binary pts, 3: timestamp_t ts_block),
  binary get_range(1: uuid_t uuid, 2: timestamp_t start_ts, 3: timestamp_t end_ts, 4: version_t version),
  binary get_range_latest(1: uuid_t uuid, 2: timestamp_t start_ts, 3: timestamp_t end_ts),
  binary get_statistical_range(1: uuid_t uuid, 2: timestamp_t start_ts, 3: timestamp_t end_ts, 4: timestamp_t resolution, 5: version_t version),
  binary get_statistical_range_latest(1: uuid_t uuid, 2: timestamp_t start_ts, 3: timestamp_t end_ts, 4: timestamp_t resolution),
  binary get_nearest_value(1: uuid_t uuid, 2: bool direction, 3: timestamp_t ts, 4: version_t version),
  binary get_nearest_value_latest(1: uuid_t uuid, 2: bool direction, 3: timestamp_t ts),
  binary compute_diff(1: uuid_t uuid, 2: version_t from_version, 3: version_t to_version),
  i64 num_entries(1: uuid_t uuid),
}

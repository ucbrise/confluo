#include "atomic_multilog.h"

namespace confluo {

atomic_multilog::atomic_multilog(const std::string &name,
                                 const std::vector<column_t> &schema,
                                 const std::string &path,
                                 const storage::storage_mode &s_mode,
                                 const archival_mode &a_mode,
                                 task_pool &pool)
    : name_(name),
      schema_(schema),
      data_log_("data_log", path, s_mode),
      rt_(path, s_mode),
      metadata_(path),
      planner_(&data_log_, &indexes_, &schema_),
      archiver_(path, rt_, &data_log_, &filters_, &indexes_, &schema_),
      archival_task_("archival"),
      archival_pool_(),
      mgmt_pool_(pool),
      monitor_task_("monitor") {
  data_log_.pre_alloc();
  metadata_.write_schema(schema_);
  metadata_.write_storage_mode(s_mode);
  metadata_.write_archival_mode(a_mode);
  monitor_task_.start(std::bind(&atomic_multilog::monitor_task, this), configuration_params::MONITOR_PERIODICITY_MS());
  if (a_mode == archival_mode::ON) {
    archival_task_.start(std::bind(&atomic_multilog::archival_task, this),
                         archival_configuration_params::PERIODICITY_MS());
  }
}

atomic_multilog::atomic_multilog(const std::string &name,
                                 const std::string &schema,
                                 const std::string &path,
                                 const storage::storage_mode &storage_mode,
                                 const archival_mode &a_mode,
                                 task_pool &pool)
    : atomic_multilog(name, parser::parse_schema(schema), path, storage_mode, a_mode, pool) {
}

atomic_multilog::atomic_multilog(const std::string &name, const std::string &path, task_pool &pool)
    : name_(name),
      schema_(),
      metadata_(path),
      planner_(&data_log_, &indexes_, &schema_),
      archiver_(path, rt_, &data_log_, &filters_, &indexes_, &schema_, false),
      archival_task_("archival"),
      archival_pool_(),
      mgmt_pool_(pool),
      monitor_task_("monitor") {
  storage_mode s_mode;
  archival_mode a_mode;
  load_metadata(path, s_mode, a_mode);
  data_log_ = data_log_type("data_log", path, s_mode);
  rt_ = read_tail_type(path, s_mode);
  load(s_mode);
  monitor_task_.start(std::bind(&atomic_multilog::monitor_task, this), configuration_params::MONITOR_PERIODICITY_MS());
  if (a_mode == archival_mode::ON) {
    archival_task_.start(std::bind(&atomic_multilog::archival_task, this),
                         archival_configuration_params::PERIODICITY_MS());
  }
}

void atomic_multilog::archive() {
  archive(rt_.get());
}

void atomic_multilog::archive(size_t offset) {
  optional<management_exception> ex;
  std::future<void> ret = archival_pool_.submit([this, offset] {
    archiver_.archive(offset);
  });
  ret.wait();
  if (ex.has_value())
    throw ex.value();
}

void atomic_multilog::add_index(const std::string &field_name, double bucket_size) {
  optional<management_exception> ex;
  std::future<void> ret = mgmt_pool_.submit(
      [field_name, bucket_size, &ex, this] {
        add_index_task(field_name, bucket_size, ex);
      });
  ret.wait();
  if (ex.has_value())
    throw ex.value();
}

void atomic_multilog::remove_index(const std::string &field_name) {
  optional<management_exception> ex;
  std::future<void> ret = mgmt_pool_.submit([field_name, &ex, this] {
    remove_index_task(field_name, ex);
  });
  ret.wait();
  if (ex.has_value())
    throw ex.value();
}

bool atomic_multilog::is_indexed(const std::string &field_name) {
  optional<management_exception> ex;
  size_t idx;
  try {
    idx = schema_.get_field_index(field_name);
  } catch (std::exception &e) {
    THROW(management_exception, "Field name does not exist");
  }
  column_t &col = schema_[idx];
  return col.is_indexed();
}

void atomic_multilog::add_filter(const std::string &name, const std::string &expr) {
  optional<management_exception> ex;
  std::future<void> ret = mgmt_pool_.submit([name, expr, &ex, this] {
    add_filter_task(name, expr, ex);
  });
  ret.wait();
  if (ex.has_value())
    throw ex.value();
}

void atomic_multilog::remove_filter(const std::string &name) {
  optional<management_exception> ex;
  std::future<void> ret = mgmt_pool_.submit([name, &ex, this] {
    remove_filter_task(name, ex);
  });
  ret.wait();
  if (ex.has_value())
    throw ex.value();
}

void atomic_multilog::add_aggregate(const std::string &name, const std::string &filter_name, const std::string &expr) {
  optional<management_exception> ex;
  std::future<void> ret = mgmt_pool_.submit(
      [name, filter_name, expr, &ex, this] {
        add_aggregate_task(name, filter_name, expr, ex);
      });
  ret.wait();
  if (ex.has_value())
    throw ex.value();
}

void atomic_multilog::remove_aggregate(const std::string &name) {
  optional<management_exception> ex;
  std::future<void> ret = mgmt_pool_.submit([name, &ex, this] {
    remove_aggregate_task(name, ex);
  });
  ret.wait();
  if (ex.has_value())
    throw ex.value();
}

void atomic_multilog::install_trigger(const std::string &name, const std::string &expr, const uint64_t periodicity_ms) {
  if (periodicity_ms < configuration_params::MONITOR_PERIODICITY_MS()) {
    throw management_exception(
        "Trigger periodicity (" + std::to_string(periodicity_ms)
            + "ms) cannot be less than monitor periodicity ("
            + std::to_string(configuration_params::MONITOR_PERIODICITY_MS())
            + "ms)");
  }

  if (periodicity_ms % configuration_params::MONITOR_PERIODICITY_MS() != 0) {
    throw management_exception(
        "Trigger periodicity (" + std::to_string(periodicity_ms)
            + "ms) must be a multiple of monitor periodicity ("
            + std::to_string(configuration_params::MONITOR_PERIODICITY_MS())
            + "ms)");
  }

  optional<management_exception> ex;
  std::future<void> ret = mgmt_pool_.submit(
      [name, expr, periodicity_ms, &ex, this] {
        add_trigger_task(name, expr, periodicity_ms, ex);
      });
  ret.wait();
  if (ex.has_value())
    throw ex.value();
}

void atomic_multilog::remove_trigger(const std::string &name) {
  optional<management_exception> ex;
  std::future<void> ret = mgmt_pool_.submit([name, &ex, this] {
    remove_trigger_task(name, ex);
  });
  ret.wait();
  if (ex.has_value())
    throw ex.value();
}

record_batch_builder atomic_multilog::get_batch_builder() const {
  return record_batch_builder(schema_);
}

size_t atomic_multilog::append_batch(record_batch &batch) {
  size_t record_size = schema_.record_size();
  size_t batch_bytes = batch.nrecords * record_size;
  size_t log_offset = data_log_.reserve(batch_bytes);
  size_t cur_offset = log_offset;
  for (record_block &block : batch.blocks) {
    data_log_.write(cur_offset,
                    reinterpret_cast<const uint8_t *>(block.data.data()),
                    block.data.length());
    update_aux_record_block(cur_offset, block, record_size);
    cur_offset += block.data.length();
  }

  data_log_.flush(log_offset, batch_bytes);
  rt_.advance(log_offset, static_cast<uint32_t>(batch_bytes));
  return log_offset;
}

size_t atomic_multilog::append(void *data) {
  size_t record_size = schema_.record_size();
  size_t offset = data_log_.append((const uint8_t *) data, record_size);
  record_t r = schema_.apply_unsafe(offset, data);

  size_t nfilters = filters_.size();
  for (size_t i = 0; i < nfilters; i++)
    if (filters_.at(i)->is_valid())
      filters_.at(i)->update(r);

  for (const field_t &f : r)
    if (f.is_indexed())
      indexes_.at(f.index_id())->insert(f.get_key(), offset);

  data_log_.flush(offset, record_size);
  rt_.advance(offset, static_cast<uint32_t>(record_size));
  return offset;
}

size_t atomic_multilog::append_json(std::string json_record) {
  void *buf = schema_.json_string_to_data(json_record);
  size_t off = append(buf);
  delete[] reinterpret_cast<uint8_t *>(buf);
  return off;
}

size_t atomic_multilog::append(const std::vector<std::string> &record) {
  void *buf = schema_.record_vector_to_data(record);
  size_t off = append(buf);
  delete[] reinterpret_cast<uint8_t *>(buf);
  return off;
}

std::unique_ptr<uint8_t> atomic_multilog::read_raw(uint64_t offset) const {
  uint64_t version;
  return read_raw(offset, version);
}

std::unique_ptr<uint8_t> atomic_multilog::read_raw(uint64_t offset, uint64_t &version) const {
  read_only_data_log_ptr rptr;
  read(offset, version, rptr);
  return rptr.decode(0, schema_.record_size());
}

void atomic_multilog::read(uint64_t offset, read_only_data_log_ptr &ptr) const {
  uint64_t version;
  read(offset, version, ptr);
}

void atomic_multilog::read(uint64_t offset, uint64_t &version, read_only_data_log_ptr &ptr) const {
  version = rt_.get();
  if (offset < version) {
    data_log_.cptr(offset, ptr);
  } else {
    ptr.init(nullptr);
  }
}

std::vector<std::string> atomic_multilog::read(uint64_t offset, uint64_t &version) const {
  read_only_data_log_ptr rptr;
  read(offset, version, rptr);
  data_ptr dptr = rptr.decode();
  return schema_.data_to_record_vector(dptr.get());
}

std::vector<std::string> atomic_multilog::read(uint64_t offset) const {
  uint64_t version;
  return read(offset, version);
}

std::string atomic_multilog::read_json(uint64_t offset, uint64_t &version) const {
  read_only_data_log_ptr rptr;
  read(offset, version, rptr);
  data_ptr dptr = rptr.decode();
  return schema_.data_to_json_string(dptr.get());
}

std::string atomic_multilog::read_json(uint64_t offset) const {
  uint64_t version;
  return read_json(offset, version);
}

std::unique_ptr<record_cursor> atomic_multilog::execute_filter(const std::string &expr) const {
  uint64_t version = rt_.get();
  auto t = parser::parse_expression(expr);
  auto cexpr = parser::compile_expression(t, schema_);
  query_plan plan = planner_.plan(cexpr);
  return plan.execute(version);
}

numeric atomic_multilog::execute_aggregate(const std::string &aggregate_expr, const std::string &filter_expr) {
  auto pa = parser::parse_aggregate(aggregate_expr);
  aggregator agg = aggregate_manager::get_aggregator(pa.agg);
  uint16_t field_idx = schema_[pa.field_name].idx();
  uint64_t version = rt_.get();
  auto t = parser::parse_expression(filter_expr);
  auto cexpr = parser::compile_expression(t, schema_);
  query_plan plan = planner_.plan(cexpr);
  return plan.aggregate(version, field_idx, agg);
}

std::unique_ptr<record_cursor> atomic_multilog::query_filter(const std::string &filter_name,
                                                             uint64_t begin_ms,
                                                             uint64_t end_ms) const {
  filter_id_t filter_id;
  if (filter_map_.get(filter_name, filter_id) == -1) {
    throw invalid_operation_exception(
        "Filter " + filter_name + " does not exist.");
  }

  filter::range_result res = filters_.at(filter_id)->lookup_range(begin_ms,
                                                                  end_ms);
  uint64_t version = rt_.get();
  std::unique_ptr<offset_cursor> o_cursor(
      new offset_iterator_cursor<filter::range_result::iterator>(res.begin(),
                                                                 res.end(),
                                                                 version));
  return std::unique_ptr<record_cursor>(
      new filter_record_cursor(std::move(o_cursor), &data_log_, &schema_,
                               parser::compiled_expression()));
}

std::unique_ptr<record_cursor> atomic_multilog::query_filter(const std::string &filter_name,
                                                             uint64_t begin_ms,
                                                             uint64_t end_ms,
                                                             const std::string &additional_filter_expr) const {
  auto t = parser::parse_expression(additional_filter_expr);
  auto e = parser::compile_expression(t, schema_);
  filter_id_t filter_id;
  if (filter_map_.get(filter_name, filter_id) == -1) {
    throw invalid_operation_exception(
        "Filter " + filter_name + " does not exist.");
  }

  filter::range_result res = filters_.at(filter_id)->lookup_range(begin_ms, end_ms);
  uint64_t version = rt_.get();
  std::unique_ptr<offset_cursor> o_cursor(
      new offset_iterator_cursor<filter::range_result::iterator>(res.begin(), res.end(), version));
  return std::unique_ptr<record_cursor>(new filter_record_cursor(std::move(o_cursor), &data_log_, &schema_, e));
}

numeric atomic_multilog::get_aggregate(const std::string &aggregate_name, uint64_t begin_ms, uint64_t end_ms) {
  aggregate_id_t aggregate_id;
  if (aggregate_map_.get(aggregate_name, aggregate_id) == -1) {
    throw invalid_operation_exception("Aggregate " + aggregate_name + " does not exist.");
  }
  uint64_t version = rt_.get();
  size_t fid = aggregate_id.filter_idx;
  size_t aid = aggregate_id.aggregate_idx;
  aggregate_info *a = filters_.at(fid)->get_aggregate_info(aid);
  numeric agg = a->zero();
  for (uint64_t t = begin_ms; t <= end_ms; t++) {
    aggregated_reflog const *refs;
    if ((refs = filters_.at(fid)->lookup(t)) == nullptr)
      continue;
    numeric t_agg = refs->get_aggregate(aid, version);
    agg = a->comb_op(agg, t_agg);
  }
  return agg;
}

std::unique_ptr<alert_cursor> atomic_multilog::get_alerts(uint64_t begin_ms, uint64_t end_ms) const {
  return get_alerts(begin_ms, end_ms, "");
}

std::unique_ptr<alert_cursor> atomic_multilog::get_alerts(uint64_t begin_ms,
                                                          uint64_t end_ms,
                                                          const std::string &trigger_name) const {
  return std::unique_ptr<alert_cursor>(new trigger_alert_cursor(alerts_.get_alerts(begin_ms, end_ms), trigger_name));
}

const std::string &atomic_multilog::get_name() const {
  return name_;
}

const schema_t &atomic_multilog::get_schema() const {
  return schema_;
}

size_t atomic_multilog::num_records() const {
  return rt_.get() / schema_.record_size();
}

size_t atomic_multilog::record_size() const {
  return schema_.record_size();
}

void atomic_multilog::load(const storage::storage_mode &mode) {
  load_utils::load_data_log(archiver_.data_log_path(), mode, data_log_);
  load_utils::load_replay_filter_log(archiver_.filter_log_path(), filters_, data_log_, schema_);
  load_utils::load_replay_index_log(archiver_.index_log_path(), indexes_, data_log_, schema_);
  rt_.advance(0, static_cast<uint32_t>(data_log_.size()));
}

void atomic_multilog::load_metadata(const std::string &path, storage_mode &s_mode, archival_mode &a_mode) {
  metadata_reader reader(path);
  metadata_writer temp = metadata_;
  metadata_ = metadata_writer(); // metadata shouldn't be written while loading
  while (reader.has_next()) {
    switch (reader.next_type()) {
      case D_SCHEMA_METADATA: {
        schema_ = reader.next_schema();
        break;
      }
      case D_FILTER_METADATA: {
        auto filter_metadata = reader.next_filter_metadata();
        add_filter(filter_metadata.filter_name(), filter_metadata.expr());
        break;
      }
      case D_INDEX_METADATA: {
        auto index_metadata = reader.next_index_metadata();
        add_index(index_metadata.field_name(), index_metadata.bucket_size());
        break;
      }
      case D_AGGREGATE_METADATA: {
        auto agg_metadata = reader.next_aggregate_metadata();
        add_aggregate(agg_metadata.aggregate_name(), agg_metadata.filter_name(), agg_metadata.aggregate_expression());
        break;
      }
      case D_TRIGGER_METADATA: {
        auto trigger_metadata = reader.next_trigger_metadata();
        optional<management_exception> ex;
        add_trigger_task(trigger_metadata.trigger_name(), trigger_metadata.trigger_expression(),
                         trigger_metadata.periodicity_ms(), ex);
        break;
      }
      case D_STORAGE_MODE_METADATA: {
        s_mode = reader.next_storage_mode();
        break;
      }
      case D_ARCHIVAL_MODE_METADATA: {
        a_mode = reader.next_archival_mode();
        break;
      }
    }
  }
  metadata_ = temp;
}

void atomic_multilog::update_aux_record_block(uint64_t log_offset, record_block &block, size_t record_size) {
  schema_snapshot snap = schema_.snapshot();
  for (size_t i = 0; i < filters_.size(); i++) {
    if (filters_.at(i)->is_valid()) {
      filters_.at(i)->update(log_offset, snap, block, record_size);
    }
  }

  for (size_t i = 0; i < schema_.size(); i++) {
    if (snap.is_indexed(i)) {
      radix_index *idx = indexes_.at(snap.index_id(i));
      // Handle timestamp differently
      // TODO: What if indexing requested for finer granularity?
      if (i == 0) {  // Timestamp
        auto &refs = idx->get_or_create(snap.time_key(block.time_block));
        size_t off = refs->reserve(block.nrecords);
        for (size_t j = 0; j < block.nrecords; j++) {
          refs->set(off + j, log_offset + j * record_size);
        }
      } else {
        for (size_t j = 0; j < block.nrecords; j++) {
          size_t block_offset = j * record_size;
          size_t record_offset = log_offset + block_offset;
          void *rec_ptr = reinterpret_cast<uint8_t *>(&block.data[0])
              + block_offset;
          idx->insert(snap.get_key(rec_ptr, static_cast<uint32_t>(i)), record_offset);
        }
      }
    }
  }
}

void atomic_multilog::add_index_task(const std::string &field_name,
                                     double bucket_size,
                                     optional<management_exception> &ex) {
  size_t idx;
  try {
    idx = schema_.get_field_index(field_name);
  } catch (std::exception &e) {
    ex = management_exception("Could not add index for " + field_name + ": " + e.what());
    return;
  }

  column_t &col = schema_[idx];
  bool success = col.set_indexing();
  if (success) {
    uint16_t index_id = UINT16_MAX;
    if (col.type().is_valid()) {
      index_id = static_cast<uint16_t>(indexes_.push_back(new radix_index(col.type().size, 256)));
    } else {
      ex = management_exception("Index not supported for field type");
    }
    col.set_indexed(index_id, bucket_size);
    metadata_.write_index_metadata(field_name, bucket_size);
  } else {
    ex = management_exception("Could not index " + field_name + ": already indexed/indexing");
    return;
  }
}

void atomic_multilog::remove_index_task(const std::string &field_name, optional<management_exception> &ex) {
  uint16_t idx;
  try {
    idx = static_cast<uint16_t>(schema_.get_field_index(field_name));
  } catch (std::exception &e) {
    ex = management_exception("Could not remove index for " + field_name + " : " + e.what());
    return;
  }

  if (!schema_[idx].disable_indexing()) {
    ex = management_exception("Could not remove index for " + field_name + ": No index exists");
    return;
  }
}

void atomic_multilog::add_filter_task(const std::string &name,
                                      const std::string &expr,
                                      optional<management_exception> &ex) {
  filter_id_t filter_id;
  if (filter_map_.get(name, filter_id) != -1) {
    ex = management_exception("Filter " + name + " already exists.");
    return;
  }
  auto t = parser::parse_expression(expr);
  auto cexpr = parser::compile_expression(t, schema_);
  filter_id = filters_.push_back(new filter(cexpr, default_filter));
  metadata_.write_filter_metadata(name, expr);
  if (filter_map_.put(name, filter_id) == -1) {
    ex = management_exception("Could not add filter " + name + " to filter map.");
    return;
  }
}

void atomic_multilog::remove_filter_task(const std::string &name, optional<management_exception> &ex) {
  filter_id_t filter_id;
  if (filter_map_.get(name, filter_id) == -1) {
    ex = management_exception("Filter " + name + " does not exist.");
    return;
  }
  bool success = filters_.at(filter_id)->invalidate();
  if (!success) {
    ex = management_exception("Filter already invalidated.");
    return;
  }
  filter_map_.remove(name, filter_id);
}

void atomic_multilog::add_aggregate_task(const std::string &name,
                                         const std::string &filter_name,
                                         const std::string &expr,
                                         optional<management_exception> &ex) {
  aggregate_id_t aggregate_id;
  if (aggregate_map_.get(name, aggregate_id) != -1) {
    ex = management_exception("Aggregate " + name + " already exists.");
    return;
  }
  filter_id_t filter_id;
  if (filter_map_.get(filter_name, filter_id) == -1) {
    ex = management_exception("Filter " + filter_name + " does not exist.");
    return;
  }
  aggregate_id.filter_idx = filter_id;
  auto pa = parser::parse_aggregate(expr);
  const column_t &col = schema_[pa.field_name];
  aggregate_info *a = new aggregate_info(name, aggregate_manager::get_aggregator(pa.agg), col.idx());
  aggregate_id.aggregate_idx = filters_.at(filter_id)->add_aggregate(a);
  if (aggregate_map_.put(name, aggregate_id) == -1) {
    ex = management_exception("Could not add trigger " + filter_name + " to trigger map.");
    return;
  }
  metadata_.write_aggregate_metadata(name, filter_name, expr);
}

void atomic_multilog::remove_aggregate_task(const std::string &name, optional<management_exception> &ex) {
  aggregate_id_t aggregate_id;
  if (aggregate_map_.get(name, aggregate_id) == -1) {
    ex = management_exception("Aggregate " + name + " does not exist.");
    return;
  }
  bool success = filters_.at(aggregate_id.filter_idx)->remove_aggregate(aggregate_id.aggregate_idx);
  if (!success) {
    ex = management_exception("Aggregate already invalidated.");
    return;
  }
  aggregate_map_.remove(name, aggregate_id);
}

void atomic_multilog::add_trigger_task(const std::string &name,
                                       const std::string &expr,
                                       uint64_t periodicity_ms,
                                       optional<management_exception> &ex) {
  trigger_id_t trigger_id;
  if (trigger_map_.get(name, trigger_id) != -1) {
    ex = management_exception("Trigger " + name + " already exists.");
    return;
  }
  auto pt = parser::parse_trigger(expr);
  std::string aggregate_name = pt.aggregate_name;
  aggregate_id_t aggregate_id;
  if (aggregate_map_.get(aggregate_name, aggregate_id) == -1) {
    ex = management_exception(
        "Aggregate " + aggregate_name + " does not exist.");
    return;
  }
  trigger_id.aggregate_id = aggregate_id;
  aggregate_info *a = filters_.at(aggregate_id.filter_idx)->get_aggregate_info(aggregate_id.aggregate_idx);
  trigger *t =
      new trigger(name, aggregate_name, relop_utils::str_to_op(pt.relop), a->value(pt.threshold), periodicity_ms);
  trigger_id.trigger_idx = a->add_trigger(t);
  if (trigger_map_.put(name, trigger_id) == -1) {
    ex = management_exception("Could not add trigger " + name + " to trigger map.");
    return;
  }
  metadata_.write_trigger_metadata(name, expr, periodicity_ms);
}

void atomic_multilog::remove_trigger_task(const std::string &name, optional<management_exception> &ex) {
  trigger_id_t trigger_id;
  if (trigger_map_.get(name, trigger_id) == -1) {
    ex = management_exception("Trigger " + name + " does not exist.");
    return;
  }
  size_t fid = trigger_id.aggregate_id.filter_idx;
  size_t aid = trigger_id.aggregate_id.aggregate_idx;
  size_t tid = trigger_id.trigger_idx;
  bool success = filters_.at(fid)->get_aggregate_info(aid)->remove_trigger(tid);
  if (!success) {
    ex = management_exception("Trigger already invalidated.");
    return;
  }
  trigger_map_.remove(name, trigger_id);
}

void atomic_multilog::archival_task() {
  optional<management_exception> ex;
  std::future<void> ret = archival_pool_.submit([this] {
    if (rt_.get() > archival_configuration_params::IN_MEMORY_DATALOG_WINDOW_BYTES())
      archiver_.archive(rt_.get() - archival_configuration_params::IN_MEMORY_DATALOG_WINDOW_BYTES());
  });
  ret.wait();
  if (ex.has_value())
    throw ex.value();
}

void atomic_multilog::monitor_task() {
  uint64_t cur_ms = time_utils::cur_ms();
  uint64_t version = rt_.get();
  size_t nfilters = filters_.size();
  for (size_t i = 0; i < nfilters; i++) {
    filter *f = filters_.at(i);
    if (f->is_valid()) {
      size_t naggs = f->num_aggregates();
      for (size_t aid = 0; aid < naggs; aid++) {
        aggregate_info *a = f->get_aggregate_info(aid);
        if (a->is_valid()) {
          size_t ntriggers = a->num_triggers();
          for (size_t tid = 0; tid < ntriggers; tid++) {
            trigger *t = a->get_trigger(tid);
            if (t->is_valid()) {
              for (uint64_t ms = cur_ms - configuration_params::MONITOR_WINDOW_MS(); ms <= cur_ms; ms++) {
                if (ms % t->periodicity_ms() == 0) {
                  check_time_bucket(f, t, tid, ms, version);
                }
              }
            }
          }
        }
      }
    }
  }
}

void atomic_multilog::check_time_bucket(filter *f, trigger *t, size_t tid, uint64_t time_bucket, uint64_t version) {
  size_t window_size = t->periodicity_ms();
  for (uint64_t ms = time_bucket - window_size; ms < time_bucket; ms++) {
    const aggregated_reflog *ar = f->lookup(ms);
    if (ar != nullptr) {
      numeric agg = ar->get_aggregate(tid, version);
      if (numeric::relop(t->op(), agg, t->threshold())) {
        alerts_.add_alert(ms, t->name(), t->expr(), agg, version);
      }
    }
  }
}

}
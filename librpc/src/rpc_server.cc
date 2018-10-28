#include "rpc_server.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace ::confluo;
using namespace ::confluo::rpc;
using namespace ::utils;

namespace confluo {
namespace rpc {

rpc_service_handler::rpc_service_handler(confluo_store *store)
    : handler_id_(-1),
      store_(store),
      iterator_id_(0) {
}
void rpc_service_handler::register_handler() {
  handler_id_ = thread_manager::register_thread();
  if (handler_id_ < 0) {
    rpc_management_exception ex;
    ex.msg = "Could not register handler";
    throw ex;
  } else {
    LOG_INFO << "Registered handler thread " << std::this_thread::get_id() << " as " << handler_id_;
  }
}
void rpc_service_handler::deregister_handler() {
  int ret = thread_manager::deregister_thread();
  if (ret < 0) {
    rpc_management_exception ex;
    ex.msg = "Could not deregister handler";
    throw ex;
  } else {
    LOG_INFO << "Deregistered handler thread " << std::this_thread::get_id() << " as " << ret;
  }
}
int64_t rpc_service_handler::create_atomic_multilog(const std::string &name,
                                                    const rpc_schema &schema,
                                                    const rpc_storage_mode mode) {
  int64_t ret;
  try {
    ret = store_->create_atomic_multilog(name,
                                         rpc_type_conversions::convert_schema(schema),
                                         rpc_type_conversions::convert_mode(mode));
  } catch (management_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  }
  return ret;
}
void rpc_service_handler::get_atomic_multilog_info(rpc_atomic_multilog_info &_return, const std::string &name) {
  _return.id = store_->get_atomic_multilog_id(name);
  auto dschema = store_->get_atomic_multilog(_return.id)->get_schema().columns();
  _return.schema = rpc_type_conversions::convert_schema(dschema);
}
void rpc_service_handler::remove_atomic_multilog(int64_t id) {
  try {
    store_->remove_atomic_multilog(id);
  } catch (management_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  }
}
void rpc_service_handler::add_index(int64_t id, const std::string &field_name, const double bucket_size) {
  try {
    store_->get_atomic_multilog(id)->add_index(field_name, bucket_size);
  } catch (management_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  }
}
void rpc_service_handler::remove_index(int64_t id, const std::string &field_name) {
  try {
    store_->get_atomic_multilog(id)->remove_index(field_name);
  } catch (management_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  }
}
void rpc_service_handler::add_filter(int64_t id, const std::string &filter_name, const std::string &filter_expr) {
  try {
    store_->get_atomic_multilog(id)->add_filter(filter_name, filter_expr);
  } catch (management_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  } catch (parse_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  }
}
void rpc_service_handler::remove_filter(int64_t id, const std::string &filter_name) {
  try {
    store_->get_atomic_multilog(id)->remove_filter(filter_name);
  } catch (management_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  }
}
void rpc_service_handler::add_aggregate(int64_t id,
                                        const std::string &aggregate_name,
                                        const std::string &filter_name,
                                        const std::string &aggregate_expr) {
  try {
    store_->get_atomic_multilog(id)->add_aggregate(aggregate_name, filter_name, aggregate_expr);
  } catch (management_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  } catch (parse_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  }
}
void rpc_service_handler::remove_aggregate(int64_t id, const std::string &aggregate_name) {
  try {
    store_->get_atomic_multilog(id)->remove_aggregate(aggregate_name);
  } catch (management_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  }
}
void rpc_service_handler::add_trigger(int64_t id, const std::string &trigger_name, const std::string &trigger_expr) {
  try {
    store_->get_atomic_multilog(id)->install_trigger(trigger_name, trigger_expr);
  } catch (management_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  } catch (parse_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  }
}
void rpc_service_handler::remove_trigger(int64_t id, const std::string &trigger_name) {
  try {
    store_->get_atomic_multilog(id)->remove_trigger(trigger_name);
  } catch (management_exception &ex) {
    rpc_management_exception e;
    e.msg = ex.what();
    throw e;
  }
}
int64_t rpc_service_handler::append(int64_t id, const std::string &data) {
  void *buf = (char *) &data[0];  // XXX: Fix
  return static_cast<int64_t>(store_->get_atomic_multilog(id)->append(buf));
}
int64_t rpc_service_handler::append_json(int64_t id, const std::string &json_data) {
  return static_cast<int64_t>(store_->get_atomic_multilog(id)->append_json(json_data));
}
int64_t rpc_service_handler::append_batch(int64_t id, const rpc_record_batch &batch) {
  record_batch rbatch = rpc_type_conversions::convert_batch(batch);
  return static_cast<int64_t>(store_->get_atomic_multilog(id)->append_batch(rbatch));
}
void rpc_service_handler::read(std::string &_return, int64_t id, const int64_t offset, const int64_t nrecords) {
  atomic_multilog *mlog = store_->get_atomic_multilog(id);
  uint64_t limit;
  read_only_data_log_ptr ptr;
  mlog->read((uint64_t) offset, limit, ptr);
  data_ptr dptr = ptr.decode();
  char *data = reinterpret_cast<char *>(dptr.get());
  size_t size = std::min(static_cast<size_t>(limit - offset),
                         static_cast<size_t>(nrecords * mlog->record_size()));
  _return.assign(data, size);
}
void rpc_service_handler::read_json(std::string &_return, int64_t id, const int64_t offset, const int64_t nrecords) {
  atomic_multilog *mlog = store_->get_atomic_multilog(id);
  _return = mlog->read_json((uint64_t) offset);
  // TODO: put in functionality for nrecords to be read
}
void rpc_service_handler::query_aggregate(std::string &_return,
                                          int64_t id,
                                          const std::string &aggregate_name,
                                          const int64_t begin_ms,
                                          const int64_t end_ms) {
  atomic_multilog *m = store_->get_atomic_multilog(id);
  _return = m->get_aggregate(aggregate_name, (uint64_t) begin_ms, (uint64_t) end_ms).to_string();
}
void rpc_service_handler::adhoc_aggregate(std::string &_return,
                                          int64_t id,
                                          const std::string &aggregate_expr,
                                          const std::string &filter_expr) {
  atomic_multilog *m = store_->get_atomic_multilog(id);
  _return = m->execute_aggregate(aggregate_expr, filter_expr).to_string();
}
void rpc_service_handler::adhoc_filter(rpc_iterator_handle &_return, int64_t id, const std::string &filter_expr) {
  bool success;
  rpc_iterator_id it_id = new_iterator_id();
  atomic_multilog *mlog = store_->get_atomic_multilog(id);
  try {
    adhoc_entry entry(it_id, mlog->execute_filter(filter_expr));
    adhoc_status ret = adhoc_.insert(std::move(entry));
    success = ret.second;
  } catch (parse_exception &ex) {
    rpc_invalid_operation e;
    e.msg = ex.what();
    throw e;
  }

  if (!success) {
    rpc_invalid_operation e;
    e.msg = "Duplicate rpc_iterator_id assigned";
    throw e;
  }

  adhoc_more(_return, mlog->record_size(), it_id);
}
void rpc_service_handler::predef_filter(rpc_iterator_handle &_return,
                                        int64_t id,
                                        const std::string &filter_name,
                                        const int64_t begin_ms,
                                        const int64_t end_ms) {
  rpc_iterator_id it_id = new_iterator_id();
  atomic_multilog *mlog = store_->get_atomic_multilog(id);
  predef_entry entry(it_id, mlog->query_filter(filter_name, (uint64_t) begin_ms, (uint64_t) end_ms));
  predef_status ret = predef_.insert(std::move(entry));
  if (!ret.second) {
    rpc_invalid_operation e;
    e.msg = "Duplicate rpc_iterator_id assigned";
    throw e;
  }

  predef_more(_return, mlog->record_size(), it_id);
}
void rpc_service_handler::combined_filter(rpc_iterator_handle &_return,
                                          int64_t id,
                                          const std::string &filter_name,
                                          const std::string &filter_expr,
                                          const int64_t begin_ms,
                                          const int64_t end_ms) {
  bool success;
  rpc_iterator_id it_id = new_iterator_id();
  atomic_multilog *mlog = store_->get_atomic_multilog(id);
  try {
    combined_entry entry(it_id, mlog->query_filter(filter_name, (uint64_t) begin_ms, (uint64_t) end_ms, filter_expr));
    combined_status ret = combined_.insert(std::move(entry));
    success = ret.second;
  } catch (parse_exception &ex) {
    rpc_invalid_operation e;
    e.msg = ex.what();
    throw e;
  }
  if (!success) {
    rpc_invalid_operation e;
    e.msg = "Duplicate rpc_iterator_id assigned";
    throw e;
  }

  combined_more(_return, mlog->record_size(), it_id);
}
void rpc_service_handler::alerts_by_time(rpc_iterator_handle &_return,
                                         int64_t id,
                                         const int64_t begin_ms,
                                         const int64_t end_ms) {
  rpc_iterator_id it_id = new_iterator_id();
  atomic_multilog *mlog = store_->get_atomic_multilog(id);
  alerts_entry entry(it_id, mlog->get_alerts((uint64_t) begin_ms, (uint64_t) end_ms));
  alerts_status ret = alerts_.insert(std::move(entry));
  if (!ret.second) {
    rpc_invalid_operation e;
    e.msg = "Duplicate rpc_iterator_id assigned";
    throw e;
  }

  alerts_more(_return, it_id);
}
void rpc_service_handler::alerts_by_trigger_and_time(rpc_iterator_handle &_return,
                                                     int64_t id,
                                                     const std::string &trigger_name,
                                                     const int64_t begin_ms,
                                                     const int64_t end_ms) {
  rpc_iterator_id it_id = new_iterator_id();
  atomic_multilog *mlog = store_->get_atomic_multilog(id);
  alerts_entry entry(it_id, mlog->get_alerts((uint64_t) begin_ms, (uint64_t) end_ms, trigger_name));
  alerts_status ret = alerts_.insert(std::move(entry));
  if (!ret.second) {
    rpc_invalid_operation e;
    e.msg = "Duplicate rpc_iterator_id assigned";
    throw e;
  }

  alerts_more(_return, it_id);
}
void rpc_service_handler::get_more(rpc_iterator_handle &_return, int64_t id, const rpc_iterator_descriptor &desc) {
  if (desc.handler_id != handler_id_) {
    rpc_invalid_operation ex;
    ex.msg = "handler_id mismatch";
    throw ex;
  }

  size_t record_size = store_->get_atomic_multilog(id)->record_size();

  switch (desc.type) {
    case rpc_iterator_type::RPC_ADHOC: {
      adhoc_more(_return, record_size, desc.id);
      break;
    }
    case rpc_iterator_type::RPC_PREDEF: {
      predef_more(_return, record_size, desc.id);
      break;
    }
    case rpc_iterator_type::RPC_COMBINED: {
      combined_more(_return, record_size, desc.id);
      break;
    }
    case rpc_iterator_type::RPC_ALERTS: {
      alerts_more(_return, desc.id);
      break;
    }
  }
}

int64_t rpc_service_handler::num_records(int64_t id) {
  return static_cast<int64_t>(store_->get_atomic_multilog(id)->num_records());
}

rpc_iterator_id rpc_service_handler::new_iterator_id() {
  return iterator_id_++;
}
void rpc_service_handler::adhoc_more(rpc_iterator_handle &_return, size_t record_size, rpc_iterator_id it_id) {
  // Initialize iterator descriptor
  _return.desc.data_type = rpc_data_type::RPC_RECORD;
  _return.desc.handler_id = handler_id_;
  _return.desc.id = it_id;
  _return.desc.type = rpc_iterator_type::RPC_ADHOC;

  // Read data from iterator
  try {
    auto &res = adhoc_.at(it_id);
    size_t to_read = rpc_configuration_params::ITERATOR_BATCH_SIZE();
    _return.data.reserve(record_size * to_read);
    size_t i = 0;
    for (; res->has_more() && i < to_read; ++i, res->advance()) {
      record_t rec = res->get();
      _return.data.append(reinterpret_cast<const char *>(rec.data()), rec.length());
    }
    _return.num_entries = static_cast<int32_t>(i);
    _return.has_more = res->has_more();
  } catch (std::out_of_range &ex) {
    rpc_invalid_operation e;
    e.msg = "No such iterator";
    throw e;
  }
}
void rpc_service_handler::predef_more(rpc_iterator_handle &_return, size_t record_size, rpc_iterator_id it_id) {
  // Initialize iterator descriptor
  _return.desc.data_type = rpc_data_type::RPC_RECORD;
  _return.desc.handler_id = handler_id_;
  _return.desc.id = it_id;
  _return.desc.type = rpc_iterator_type::RPC_PREDEF;

  // Read data from iterator
  try {
    auto &res = predef_.at(it_id);
    size_t to_read = rpc_configuration_params::ITERATOR_BATCH_SIZE();
    _return.data.reserve(record_size * to_read);
    size_t i = 0;
    for (; res->has_more() && i < to_read; ++i, res->advance()) {
      record_t rec = res->get();
      _return.data.append(reinterpret_cast<const char *>(rec.data()), rec.length());
    }
    _return.num_entries = static_cast<int32_t>(i);
    _return.has_more = res->has_more();
  } catch (std::out_of_range &ex) {
    rpc_invalid_operation e;
    e.msg = "No such iterator";
    throw e;
  }
}
void rpc_service_handler::combined_more(rpc_iterator_handle &_return, size_t record_size, rpc_iterator_id it_id) {
  // Initialize iterator descriptor
  _return.desc.data_type = rpc_data_type::RPC_RECORD;
  _return.desc.handler_id = handler_id_;
  _return.desc.id = it_id;
  _return.desc.type = rpc_iterator_type::RPC_COMBINED;

  // Read data from iterator
  try {
    auto &res = combined_.at(it_id);
    size_t to_read = rpc_configuration_params::ITERATOR_BATCH_SIZE();
    _return.data.reserve(record_size * to_read);
    size_t i = 0;
    for (; res->has_more() && i < to_read; ++i, res->advance()) {
      record_t rec = res->get();
      _return.data.append(reinterpret_cast<const char *>(rec.data()), rec.length());
    }
    _return.num_entries = static_cast<int32_t>(i);
    _return.has_more = res->has_more();
  } catch (std::out_of_range &ex) {
    rpc_invalid_operation e;
    e.msg = "No such iterator";
    throw e;
  }
}
void rpc_service_handler::alerts_more(rpc_iterator_handle &_return, rpc_iterator_id it_id) {
  // Initialize iterator descriptor
  _return.desc.data_type = rpc_data_type::RPC_ALERT;
  _return.desc.handler_id = handler_id_;
  _return.desc.id = it_id;
  _return.desc.type = rpc_iterator_type::RPC_ALERTS;

  // Read data from iterator
  try {
    auto &res = alerts_.at(it_id);
    size_t to_read = rpc_configuration_params::ITERATOR_BATCH_SIZE();
    size_t i = 0;
    for (; res->has_more() && i < to_read; ++i, res->advance()) {
      alert a = res->get();
      _return.data.append(a.to_string());
      _return.data.push_back('\n');
    }
    _return.num_entries = static_cast<int32_t>(i);
    _return.has_more = res->has_more();
  } catch (std::out_of_range &ex) {
    rpc_invalid_operation e;
    e.msg = "No such iterator";
    throw e;
  }
}

rpc_clone_factory::rpc_clone_factory(confluo_store *store)
    : store_(store) {
}
rpc_clone_factory::~rpc_clone_factory() {
}
rpc_serviceIf *rpc_clone_factory::getHandler(const TConnectionInfo &conn_info) {
  std::shared_ptr<TSocket> sock = std::dynamic_pointer_cast<TSocket>(
      conn_info.transport);
  LOG_INFO << "Incoming connection\n"
           << "\t\t\tSocketInfo: " << sock->getSocketInfo() << "\n"
           << "\t\t\tPeerHost: " << sock->getPeerHost() << "\n"
           << "\t\t\tPeerAddress: " << sock->getPeerAddress() << "\n"
           << "\t\t\tPeerPort: " << sock->getPeerPort();
  return new rpc_service_handler(store_);
}
void rpc_clone_factory::releaseHandler(rpc_serviceIf *handler) {
  delete handler;
}
std::shared_ptr<TThreadedServer> rpc_server::create(confluo_store *store, const std::string &address, int port) {
  std::shared_ptr<rpc_clone_factory> clone_factory(new rpc_clone_factory(store));
  std::shared_ptr<rpc_serviceProcessorFactory> proc_factory(new rpc_serviceProcessorFactory(clone_factory));
  std::shared_ptr<TServerSocket> sock(new TServerSocket(address, port));
  std::shared_ptr<TBufferedTransportFactory> t_factory(new TBufferedTransportFactory());
  std::shared_ptr<TBinaryProtocolFactory> p_factory(new TBinaryProtocolFactory());
  std::shared_ptr<TThreadedServer> server(new TThreadedServer(proc_factory, sock, t_factory, p_factory));
  return server;
}
}
}




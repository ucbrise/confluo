#ifndef RPC_DIALOG_CLIENT_H_
#define RPC_DIALOG_CLIENT_H_

#include "rpc_dialog_client.h"

#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace dialog {
namespace rpc {

class rpc_dialog_reader : public rpc_dialog_client {
 public:
  rpc_dialog_reader()
      : rpc_dialog_client(),
        read_buffer_(std::make_pair(INT64_C(-1), "")) {
  }

  rpc_dialog_reader(const std::string& host, int port,
                    const std::string& table_name)
      : read_buffer_(std::make_pair(INT64_C(-1), "")) {
    connect(host, port);
    set_current_table(table_name);
  }

  ~rpc_dialog_reader() {
  }

  /** Query ops **/
  // Read op
  void read(std::string& _return, int64_t offset) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    int64_t& buf_off = read_buffer_.first;
    std::string& buf = read_buffer_.second;
    int64_t rbuf_lim = buf_off + buf.size();
    if (buf_off == -1 || offset < buf_off || offset >= rbuf_lim) {
      read_buffer_.first = offset;
      client_->read(buf, buf_off, rpc_configuration_params::READ_BATCH_SIZE);
    }
    _return = buf.substr(offset - buf_off, cur_schema_.record_size());
  }

  rpc_record_stream adhoc_filter(const std::string& filter_expr) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    rpc_iterator_handle handle;
    client_->adhoc_filter(handle, filter_expr);
    return rpc_record_stream(cur_schema_, client_, std::move(handle));
  }

  rpc_record_stream predef_filter(const std::string& filter_name,
                                  const int64_t begin_ms,
                                  const int64_t end_ms) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    rpc_iterator_handle handle;
    client_->predef_filter(handle, filter_name, begin_ms, end_ms);
    return rpc_record_stream(cur_schema_, client_, std::move(handle));
  }

  rpc_record_stream combined_filter(const std::string& filter_name,
                                    const std::string& filter_expr,
                                    const int64_t begin_ms,
                                    const int64_t end_ms) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    rpc_iterator_handle handle;
    client_->combined_filter(handle, filter_name, filter_expr, begin_ms,
                             end_ms);
    return rpc_record_stream(cur_schema_, client_, std::move(handle));
  }

  rpc_alert_stream get_alerts(const int64_t begin_ms, const int64_t end_ms) {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    rpc_iterator_handle handle;
    client_->alerts_by_time(handle, begin_ms, end_ms);
    return rpc_alert_stream(client_, std::move(handle));
  }

  int64_t num_records() {
    if (!table_set_) {
      throw illegal_state_exception("Must set table first");
    }
    return client_->num_records();
  }

 private:
  std::pair<int64_t, std::string> read_buffer_;
};

}
}

#endif /* RPC_DIALOG_CLIENT_H_ */

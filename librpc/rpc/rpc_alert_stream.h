#ifndef RPC_RPC_ALERT_STREAM_H_
#define RPC_RPC_ALERT_STREAM_H_

#include <sstream>

#include "rpc_service.h"

namespace confluo {
namespace rpc {

/**
 * Wrapper for the alert stream
 */
class rpc_alert_stream {
 public:
  /** The rpc client type */
  typedef rpc_serviceClient rpc_client;

  /**
   * Constructs an alert stream for the rpc client
   *
   * @param table_id The identifier for the table
   * @param client The rpc client
   * @param handle The data for the stream
   */
  rpc_alert_stream(int64_t table_id, std::shared_ptr<rpc_client> client, rpc_iterator_handle&& handle);

  /**
   * Gets the alert
   *
   * @return String containing the alert
   */
  const std::string& get() const;

  /**
   * Advances the alert stream
   *
   * @return This updated rpc alert stream 
   */
  rpc_alert_stream& operator++();

  /**
   * Checks whether there is any more elements in the stream
   *
   * @return True if the stream or handle has any more elements, false
   * otherwise
   */
  bool has_more() const;

  /**
   * Checks whether the alert stream is empty
   *
   * @return True if the alert stream is empty, false otherwise
   */
  bool empty() const;

 private:
  int64_t table_id_;
  rpc_iterator_handle handle_;
  std::stringstream stream_;
  std::string alert_;
  std::shared_ptr<rpc_client> client_;
};

}
}

#endif /* RPC_RPC_ALERT_STREAM_H_ */

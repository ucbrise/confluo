#ifndef GRAPHSTORE_CLIENT_GRAPH_CLIENT_H_
#define GRAPHSTORE_CLIENT_GRAPH_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "server/graph_store_service.h"

#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

namespace graphstore {

template<typename gs_client>
class graph_store_client {
 public:
  graph_store_client() = default;

  graph_store_client(const std::string& host, const int port) {
    connect(host, port);
  }

  graph_store_client(const graph_store_client<gs_client>& other) {
    socket_ = other.socket_;
    transport_ = other.transport_;
    protocol_ = other.protocol_;
    client_ = other.client_;
  }

  ~graph_store_client() {
    disconnect();
  }

  void connect(const std::string& host, int port) {
    socket_ = boost::shared_ptr<TSocket>(new TSocket(host, port));
    transport_ = boost::shared_ptr<TTransport>(new TBufferedTransport(socket_));
    protocol_ = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
    client_ = boost::shared_ptr<gs_client>(new gs_client(protocol_));
    transport_->open();
  }

  void disconnect() {
    if (transport_->isOpen())
    transport_->close();
  }

  void init_connection() {
    client_->init_connection();
  }

  void destroy_connection() {
    client_->destroy_connection();
  }

  int64_t add_node(const TNode& n) {
    return client_->add_node(n);
  }

  void get_node(TNode& _return, const int64_t type, const int64_t id) {
    client_->get_node(_return, type, id);
  }

  bool update_node(const TNode& n) {
    return client_->update_node(n);
  }

  bool delete_node(const int64_t type, const int64_t id) {
    return client_->delete_node(type, id);
  }

  bool add_link(const TLink& a) {
    return client_->add_link(a);
  }

  bool update_link(const TLink& a) {
    return client_->update_link(a);
  }

  bool delete_link(const int64_t id1, const int64_t link_type,
      const int64_t id2) {
    return client_->delete_link(id1, link_type, id2);
  }

  void get_link(TLink& _return, const int64_t id1, const int64_t link_type,
      const int64_t id2) {
    client_->get_link(_return, id1, link_type, id2);
  }

  void multiget_link(std::vector<TLink> & _return, const int64_t id1,
      const int64_t link_type, const std::set<int64_t> & id2s) {
    client_->multiget_link(_return, id1, link_type, id2s);
  }

  void get_link_list(std::vector<TLink> & _return, const int64_t id1,
      const int64_t link_type) {
    client_->get_link_list(_return, id1, link_type);
  }

  void get_link_list_range(std::vector<TLink> & _return, const int64_t id1,
      const int64_t link_type, const int64_t min_ts,
      const int64_t max_ts, const int64_t off,
      const int64_t limit) {
    client_->get_link_list_range(_return, id1, link_type, min_ts, max_ts, off, limit);
  }

  int64_t count_links(const int64_t id1, const int64_t link_type) {
    return client_->count_links(id1, link_type);
  }

  int64_t begin_snapshot() {
    return client_->begin_snapshot();
  }

  bool end_snapshot(const int64_t tail) {
    return client_->end_snapshot(tail);
  }

  void send_begin_snapshot() {
    client_->send_begin_snapshot();
  }

  int64_t recv_begin_snapshot() {
    return client_->recv_begin_snapshot();
  }

  void send_end_snapshot(int64_t tail) {
    client_->send_end_snapshot(tail);
  }

  bool recv_end_snapshot() {
    return client_->recv_end_snapshot();
  }

  void traverse(std::vector<TLink>& _return, const int64_t id,
      const int64_t link_type, const int64_t depth, const int64_t breadth,
      const std::vector<int64_t>& snapshot) {
    client_->traverse(_return, id, link_type, depth, breadth, snapshot, {id});
  }

  void traverse(std::vector<TLink>& _return, const int64_t id,
      const int64_t link_type, const int64_t depth, const int64_t breadth,
      const std::vector<int64_t>& snapshot, const std::set<int64_t>& visited) {
    client_->traverse(_return, id, link_type, depth,  breadth, snapshot, visited);
  }

protected:
  boost::shared_ptr<TSocket> socket_;
  boost::shared_ptr<TTransport> transport_;
  boost::shared_ptr<TProtocol> protocol_;
  boost::shared_ptr<gs_client> client_;
};

class graph_client : public graph_store_client<GraphStoreServiceClient> {
 public:
  graph_client()
      : graph_store_client<GraphStoreServiceClient>() {
  }

  graph_client(const std::string& host, const int port)
      : graph_store_client<GraphStoreServiceClient>(host, port) {
  }

  graph_client(const graph_client& client)
      : graph_store_client<GraphStoreServiceClient>(client) {
  }

  void send_traverse(const int64_t id, const int64_t link_type,
                     const int64_t depth, const int64_t breadth,
                     const std::vector<int64_t>& snapshot,
                     std::set<int64_t>& visited) {
    client_->send_traverse(id, link_type, depth, breadth, snapshot, visited);
  }

  void recv_traverse(std::vector<TLink>& _return) {
    client_->recv_traverse(_return);
  }
};

class concurrent_graph_client : public graph_store_client<
    GraphStoreServiceConcurrentClient> {
 public:
  concurrent_graph_client()
      : graph_store_client<GraphStoreServiceConcurrentClient>() {
  }

  concurrent_graph_client(const std::string& host, const int port)
      : graph_store_client<GraphStoreServiceConcurrentClient>(host, port) {
  }

  concurrent_graph_client(const concurrent_graph_client& client)
      : graph_store_client<GraphStoreServiceConcurrentClient>(client) {
  }

  int32_t send_traverse(const int64_t id, const int64_t link_type,
                        const int64_t depth, const int64_t breadth,
                        const std::vector<int64_t>& snapshot,
                        std::set<int64_t>& visited) {
    return client_->send_traverse(id, link_type, depth, breadth, snapshot, visited);
  }

  void recv_traverse(std::vector<TLink>& _return, int32_t seq_id) {
    client_->recv_traverse(_return, seq_id);
  }
};

}

#endif /* GRAPHSTORE_CLIENT_GRAPH_CLIENT_H_ */

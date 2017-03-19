#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "server/graph_store_service.h"
#include "graph_store.h"
#include "cmd_parse.h"
#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace ::graphstore;
using namespace ::datastore;

template<typename tail_scheme>
class graph_store_service : virtual public GraphStoreServiceIf {
 public:
  graph_store_service(graph_store<tail_scheme>* store) {
    store_ = store;
  }

  int64_t add_node(const TNode& n) {
    return store_->add_node(tnode_to_node_op(n));
  }

  void get_node(TNode& _return, const int64_t type, const int64_t id) {
    _return = node_op_to_tnode(store_->get_node(type, id));
  }

  bool update_node(const TNode& n) {
    return store_->update_node(tnode_to_node_op(n));
  }

  bool delete_node(const int64_t type, const int64_t id) {
    return store_->delete_node(type, id);
  }

  bool add_link(const TLink& a) {
    return store_->add_link(tlink_to_link_op(a));
  }

  bool update_link(const TLink& a) {
    return store_->update_link(tlink_to_link_op(a));
  }

  bool delete_link(const int64_t id1, const int64_t link_type,
                   const int64_t id2) {
    return store_->delete_link(id1, link_type, id2);
  }

  void get_link(TLink& _return, const int64_t id1, const int64_t link_type,
                const int64_t id2) {
    _return = link_op_to_tlink(store_->get_link(id1, link_type, id2));
  }

  void multiget_link(std::vector<TLink> & _return, const int64_t id1,
                     const int64_t link_type, const std::set<int64_t> & id2s) {
    auto res = store_->multiget_links(id1, link_type, id2s);
    for (const link_op& op : res)
      _return.push_back(link_op_to_tlink(op));
  }

  void get_link_list(std::vector<TLink> & _return, const int64_t id1,
                     const int64_t link_type) {
    auto res = store_->get_link_list(id1, link_type);
    for (const link_op& op : res)
      _return.push_back(link_op_to_tlink(op));
  }

  void get_link_list_range(std::vector<TLink> & _return, const int64_t id1,
                           const int64_t link_type, const int64_t min_ts,
                           const int64_t max_ts, const int64_t off,
                           const int64_t limit) {
    auto res = store_->get_link_list(id1, link_type, min_ts, max_ts, off,
                                     limit);
    for (const link_op& op : res)
      _return.push_back(link_op_to_tlink(op));
  }

  int64_t count_links(const int64_t id1, const int64_t link_type) {
    return store_->count_links(id1, link_type);
  }

 private:
  TNode node_op_to_tnode(const node_op& op) {
    TNode n;
    n.id = op.id;
    n.type = op.type;
    n.data = op.data;
    return n;
  }

  TLink link_op_to_tlink(const link_op& op) {
    TLink l;
    l.id1 = op.id1;
    l.link_type = op.link_type;
    l.id2 = op.id2;
    l.time = op.time;
    l.data = op.data;
    return l;
  }

  node_op tnode_to_node_op(const TNode& n) {
    node_op op;
    op.id = n.id;
    op.type = n.type;
    op.data = n.data;
    return op;
  }

  link_op tlink_to_link_op(const TLink& l) {
    link_op op;
    op.id1 = l.id1;
    op.link_type = l.link_type;
    op.id2 = l.id2;
    op.time = l.time;
    op.data = l.data;
    return op;
  }

  graph_store<tail_scheme> *store_;
};

template<typename tail_scheme>
class gs_processor_factory : public TProcessorFactory {
 public:
  gs_processor_factory(graph_store<tail_scheme>* store) {
    LOG_INFO << "Initializing processor factory...";
    store_ = store;
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    LOG_INFO << "Creating new processor...";
    boost::shared_ptr<graph_store_service<tail_scheme>> handler(
        new graph_store_service<tail_scheme>(store_));
    boost::shared_ptr<TProcessor> processor(
        new GraphStoreServiceProcessor(handler));
    return processor;
  }

 private:
  graph_store<tail_scheme> *store_;
};

template<typename tail_scheme>
void start_server(int port, graph_store<tail_scheme>* store) {
  try {
    shared_ptr<gs_processor_factory<tail_scheme>> handler_factory(
        new gs_processor_factory<tail_scheme>(store));
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(handler_factory, server_transport, transport_factory,
                           protocol_factory);

    LOG_INFO << "Listening for connections on port " << port;
    server.serve();
  } catch (std::exception& e) {
    LOG_ERROR << "Could not start server listening on port " << port << ":" << e.what();
  }
}

int main(int argc, char **argv) {

  cmd_options opts;
  opts.add(
      cmd_option("port", 'p', false).set_default("9090").set_description(
          "Port that server listens on"));
  opts.add(
      cmd_option("tail-scheme", 's', false).set_default("read-stalled")
          .set_description("Scheme for graph tail"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int port;
  std::string tail_scheme;
  try {
    port = parser.get_int("port");
    tail_scheme = parser.get("tail-scheme");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  if (tail_scheme == "write-stalled") {
    graph_store<write_stalled_tail>* store = new graph_store<write_stalled_tail>();
    start_server(port, store);
  } else if (tail_scheme == "read-stalled") {
    graph_store<read_stalled_tail>* store = new graph_store<read_stalled_tail>();
    start_server(port, store);
  } else {
    fprintf(stderr, "Unknown tail scheme: %s\n", tail_scheme.c_str());
  }

  return 0;
}


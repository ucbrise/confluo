#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <future>
#include "server/graph_store_service.h"
#include "graph_store.h"
#include "cmd_parse.h"
#include "logger.h"
#include "error_handling.h"

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
  graph_store_service(graph_store<tail_scheme>* store,
                      const std::vector<std::string> hostlist,
                      uint32_t store_id)
      : store_id_(store_id),
        hostlist_(hostlist),
        store_(store) {
  }

  void init_connection() {
    for (size_t i = 0; i < hostlist_.size(); i++) {
      if (i != store_id_)
        add_connection(i, hostlist_[i], 9090);
    }
  }

  void destroy_connection() {
    for (size_t i = 0; i < hostlist_.size(); i++) {
      if (i != store_id_ && transports_[i]->isOpen())
        transports_[i]->close();
    }
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

  int64_t begin_snapshot() {
    return store_->begin_snapshot();
  }

  bool end_snapshot(const int64_t tail) {
    return store_->end_snapshot(tail);
  }

  std::future<std::vector<TLink>> continue_traverse(
      const int64_t store_id, const int64_t id, const int64_t link_type,
      const int64_t depth, const std::vector<int64_t>& snapshot) {

    if (store_id == store_id_) {
      auto t = [id, link_type, depth, snapshot, this]() {
        std::vector<TLink> links;
        this->traverse(links, id, link_type, depth, snapshot);
        return links;
      };
      return std::async(std::launch::async, t);
    }

    clients_[store_id]->send_traverse(id, link_type, depth, snapshot);
    auto t = [store_id, id, link_type, depth, snapshot, this]() {
      std::vector<TLink> links;
      clients_[store_id]->recv_traverse(links);
      return links;
    };
    return std::async(std::launch::async, t);
  }

  void traverse(std::vector<TLink>& _return, const int64_t id,
                const int64_t link_type, const int64_t depth,
                const std::vector<int64_t>& snapshot) {
    if (depth == 0)
      return;

    uint64_t tail = snapshot[store_id_];
    std::vector<link_op> links = store_->get_links(id, link_type, tail);
    typedef std::future<std::vector<TLink>> future_t;
    std::vector<future_t> downstream_links(links.size());
    for (const link_op& op : links) {
      _return.push_back(link_op_to_tlink(op));
      downstream_links.push_back(
          continue_traverse(op.id2 / hostlist_.size(), op.id2, link_type,
                            depth - 1, snapshot));
    }

    for (future_t& f : downstream_links) {
      std::vector<TLink> ret = f.get();
      _return.insert(_return.end(), ret.begin(), ret.end());
    }
  }

 private:
  void add_connection(uint32_t i, const std::string& hostname, int port) {
    LOG_INFO<< "Connecting to " << hostname << ":" << port;
    sockets_[i] = boost::shared_ptr<TSocket>(new TSocket(hostname, port));
    transports_[i] = boost::shared_ptr<TTransport>(new TBufferedTransport(sockets_[i]));
    protocols_[i] = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transports_[i]));
    clients_[i] = boost::shared_ptr<GraphStoreServiceClient>(new GraphStoreServiceClient(protocols_[i]));
    transports_[i]->open();
  }

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

  std::map<uint32_t, boost::shared_ptr<TSocket>> sockets_;
  std::map<uint32_t, boost::shared_ptr<TTransport>> transports_;
  std::map<uint32_t, boost::shared_ptr<TProtocol>> protocols_;
  std::map<uint32_t, boost::shared_ptr<GraphStoreServiceClient>> clients_;
  atomic::type<bool> connected_;
  uint32_t store_id_;
  std::vector<std::string> hostlist_;
  graph_store<tail_scheme> *store_;
};

template<typename tail_scheme>
class gs_processor_factory : public TProcessorFactory {
 public:
  gs_processor_factory(graph_store<tail_scheme>* store,
                       const std::vector<std::string> hostlist,
                       uint32_t store_id)
      : store_(store),
        store_id_(store_id),
        hostlist_(hostlist) {
    LOG_INFO<< "Initializing processor factory...";
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    LOG_INFO<< "Creating new processor...";
    boost::shared_ptr<graph_store_service<tail_scheme>> handler(
        new graph_store_service<tail_scheme>(store_, hostlist_, store_id_));
    boost::shared_ptr<TProcessor> processor(
        new GraphStoreServiceProcessor(handler));
    return processor;
  }

private:
  graph_store<tail_scheme> *store_;
  uint32_t store_id_;
  std::vector<std::string> hostlist_;
};

template<typename tail_scheme>
void start_server(int port, graph_store<tail_scheme>* store,
                  const std::vector<std::string>& hostlist, uint32_t store_id) {
  try {
    shared_ptr<gs_processor_factory<tail_scheme>> handler_factory(
        new gs_processor_factory<tail_scheme>(store, hostlist, store_id));
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(handler_factory, server_transport, transport_factory,
                           protocol_factory);

    LOG_INFO<< "Listening for connections on port " << port;
    server.serve();
  } catch (std::exception& e) {
    LOG_ERROR<< "Could not start server listening on port " << port << ":" << e.what();
  }
}

std::vector<std::string> read_hosts(const std::string& hostfile) {
  std::vector<std::string> hostlist;
  std::ifstream in(hostfile);
  std::copy(std::istream_iterator<std::string>(in),
            std::istream_iterator<std::string>(), std::back_inserter(hostlist));
  return hostlist;
}

int main(int argc, char **argv) {

  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);

  cmd_options opts;
  opts.add(
      cmd_option("port", 'p', false).set_default("9090").set_description(
          "Port that server listens on"));
  opts.add(
      cmd_option("concurrency-control", 'c', false).set_default("read-stalled")
          .set_description("Scheme for graph tail"));
  opts.add(
      cmd_option("host-list", 'h', false).set_default("conf/hosts")
          .set_description("File containing list of hosts"));
  opts.add(
      cmd_option("server-id", 's', false).set_default("0").set_description(
          "Server ID"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int port;
  int server_id;
  std::string tail_scheme;
  std::string host_file;
  try {
    port = parser.get_int("port");
    tail_scheme = parser.get("tail-scheme");
    host_file = parser.get("host-list");
    server_id = parser.get_int("server-id");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  if (tail_scheme == "write-stalled") {
    graph_store<write_stalled>* store = new graph_store<write_stalled>();
    start_server(port, store, read_hosts(host_file), server_id);
  } else if (tail_scheme == "read-stalled") {
    graph_store<read_stalled>* store = new graph_store<read_stalled>();
    start_server(port, store, read_hosts(host_file), server_id);
  } else {
    fprintf(stderr, "Unknown tail scheme: %s\n", tail_scheme.c_str());
  }

  return 0;
}


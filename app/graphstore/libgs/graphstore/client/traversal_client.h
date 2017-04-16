#ifndef GRAPHSTORE_CLIENT_TRAVERSAL_CLIENT_H_
#define GRAPHSTORE_CLIENT_TRAVERSAL_CLIENT_H_

#include "client/graph_client.h"
#include "coordinator.h"

using namespace ::datastore;

namespace graphstore {

class traversal_client {
 public:

  traversal_client()
      : port_(9090),
        coord_(nullptr) {
  }

  traversal_client(const std::vector<std::string>& hosts, const int port,
                   const int64_t sleep_us)
      : port_(port),
        hosts_(hosts) {
    setup(hosts, port, sleep_us);
  }

  ~traversal_client() {
    LOG_INFO<<"Stopping coordinator...";
    coord_->stop();
  }

  void setup(const std::vector<std::string>& hosts, const int port,
      const int64_t sleep_us) {
    clients_.clear();
    clients_.resize(hosts.size());
    for (size_t i = 0; i < hosts.size(); i++) {
      clients_[i].connect(hosts[i], port);
      LOG_INFO << "Initializing connection to other hosts for the session...";
      clients_[i].init_connection();
    }
    LOG_INFO << "Creating coordinator...";
    coord_ = new coordinator<graph_store_client>(clients_, sleep_us);
    LOG_INFO << "Starting coordinator...";
    coord_->start();
    LOG_INFO << "Coordinator started.";
  }

  void traverse(std::vector<TLink>& _return, const int64_t id,
      const int64_t link_type, const int64_t depth) {

    const snapshot& s = coord_->get_snapshot();
    LOG_INFO << "Got snapshot: " << s.to_string();
    std::vector<int64_t> snapshots;
    for (const auto& t : s.tails)
    snapshots.push_back(t);
    size_t client_id = id % hosts_.size();
    LOG_INFO << "Forwarding request to client#" << client_id << ": " << hosts_.at(client_id) << ":" << port_;
    clients_.at(client_id).traverse(_return, id, link_type, depth, snapshots);
  }

private:
  int port_;
  std::vector<std::string> hosts_;
  std::vector<graph_store_client> clients_;
  coordinator<graph_store_client>* coord_;
};

}

#endif /* GRAPHSTORE_CLIENT_TRAVERSAL_CLIENT_H_ */

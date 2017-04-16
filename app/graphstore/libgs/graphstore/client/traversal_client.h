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
                   const int64_t sleep_us) {
    setup(hosts, port, sleep_us);
  }

  ~traversal_client() {
    LOG_INFO<<"Stopping coordinator...";
    coord_->stop();
  }

  void setup(const std::vector<std::string>& hosts, const int port,
      const int64_t sleep_us) {
    hosts_ = hosts;
    port_ = port;
    clients_.clear();
    clients_.resize(hosts.size());
    for (size_t i = 0; i < hosts.size(); i++) {
      clients_[i].connect(hosts[i], port);
      LOG_INFO << "Initializing connection to other hosts for the session...";
      clients_[i].init_connection();
    }
    LOG_INFO << "Creating coordinator...";
    coord_ = new coordinator<graph_client>(clients_, sleep_us);
  }

  void traverse(std::vector<TLink>& _return, const int64_t id,
      const int64_t link_type, const int64_t depth, const int64_t breadth) {

    uint64_t t1 = utils::time_utils::cur_ns();
    const snapshot& s = coord_->force_snapshot();
    std::vector<int64_t> snapshots;
    for (const auto& t : s.tails) {
      snapshots.push_back(t);
    }
    uint64_t t2 = utils::time_utils::cur_ns();
    size_t client_id = id % hosts_.size();
    uint64_t t3 = utils::time_utils::cur_ns();
    clients_.at(client_id).traverse(_return, id, link_type, depth, breadth, snapshots);
    uint64_t t4 = utils::time_utils::cur_ns();
    LOG_INFO << (t2-t1) << "\t" << (t4 - t3) << "\t" << _return.size();
  }

private:
  int port_;
  std::vector<std::string> hosts_;
  std::vector<graph_client> clients_;
  coordinator<graph_client>* coord_;
};

}

#endif /* GRAPHSTORE_CLIENT_TRAVERSAL_CLIENT_H_ */

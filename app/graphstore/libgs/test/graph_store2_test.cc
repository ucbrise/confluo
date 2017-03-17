#include "graph_store.h"
#include "gtest/gtest.h"

#include <thread>

using namespace graphstore;

class GraphStore2Test : public testing::Test {
 public:
  static const size_t kMaxThreads = 4;

  template<typename Work>
  void mt_test(const size_t num_threads, Work&& func) {
    std::vector<std::thread> workers;
    for (size_t i = 0; i < num_threads; i++)
      workers.push_back(std::thread(func, i));

    for (std::thread& worker : workers)
      worker.join();
  }
};

TEST_F(GraphStore2Test, AddGetNodeTest) {
  static const uint64_t kNumNodes = 1000;
  for (size_t num_threads = 1; num_threads <= kMaxThreads; num_threads++) {
    graph_store<> gs;

    mt_test(num_threads, [&](size_t thread_id) -> void {
      for (uint64_t i = 0; i < kNumNodes; i++) {
        node_op n;
        n.type = 0;
        n.data = std::to_string(kNumNodes * thread_id + i);
        gs.add_node(n);
      }
    });

    std::vector<size_t> num_nodes(num_threads, 0);
    for (uint64_t i = 0; i < num_threads * kNumNodes; i++) {
      node_op n = gs.get_node(0, i);
      ASSERT_TRUE(n.id != UINT64_MAX);
      uint64_t id = std::stoll(n.data);
      num_nodes[id / kNumNodes]++;
    }

    for (uint64_t i = 0; i < num_threads; i++)
      ASSERT_EQ(kNumNodes, num_nodes[i]);
  }
}

TEST_F(GraphStore2Test, UpdateNodeTest) {
  static const uint64_t kNumNodes = 1000;
  for (size_t num_threads = 1; num_threads <= kMaxThreads; num_threads++) {
    graph_store<> gs;

    for (uint64_t i = 0; i < kNumNodes; i++) {
      node_op n;
      n.id = i;
      n.type = 0;
      n.data = std::to_string(i);
      uint64_t id = gs.add_node(n);
      ASSERT_EQ(i, id);
    }

    mt_test(num_threads, [&](size_t thread_id) -> void {
      for (uint64_t i = 0; i < kNumNodes; i++) {
        node_op n;
        n.id = i;
        n.type = 0;
        n.data = std::to_string(kNumNodes * thread_id + i);
        bool success = gs.update_node(n);
        ASSERT_TRUE(success);
      }
    });

    for (uint64_t i = 0; i < kNumNodes; i++) {
      node_op n = gs.get_node(0, i);
      ASSERT_TRUE(n.id != UINT64_MAX);
    }
  }
}

TEST_F(GraphStore2Test, AddGetLinkTest) {
  static const uint64_t kNumNodes = 1000;
  static const uint64_t kDegree = 10;
  static const uint64_t kNumLinks = 10000;

  for (size_t num_threads = 1; num_threads <= kMaxThreads; num_threads++) {
    graph_store<> gs;
    for (uint64_t i = 0; i < kNumNodes; i++) {
      node_op n;
      n.id = i;
      n.type = i % 2;
      n.data = std::to_string(i);
      uint64_t id = gs.add_node(n);
      ASSERT_EQ(i, id);
    }

    mt_test(num_threads, [&](size_t thread_id) -> void {
      for (uint64_t i = 0; i < kNumNodes; i++) {
        for (uint64_t j = 1; j <= kDegree; j++) {
          link_op l;
          l.id1 = i;
          l.id2 = (i + j) % kNumNodes;
          l.link_type = j;
          l.time = kNumLinks - (i * kDegree + j);
          l.data = std::to_string(i) + "->" + std::to_string(j);
          bool success = gs.add_link(l);
          ASSERT_TRUE(success);
        }
      }
    });

    for (uint64_t i = 0; i < kNumNodes; i++) {
      for (uint64_t j = 1; j <= kDegree; j++) {
        link_op l = gs.get_link(i, j, (i + j) % kNumNodes);
        ASSERT_EQ(static_cast<int64_t>(i), l.id1);
        ASSERT_EQ(static_cast<int64_t>(j), l.link_type);
        ASSERT_EQ(static_cast<int64_t>((i + j) % kNumNodes), l.id2);
        ASSERT_EQ(static_cast<int64_t>(kNumLinks - (i * kDegree + j)), l.time);
        ASSERT_EQ(std::to_string(i) + "->" + std::to_string(j), l.data);
      }
    }
  }
}

TEST_F(GraphStore2Test, AddNodeLinkTest) {
  static const uint64_t kNumNodes = 1000;
  static const uint64_t kDegree = 10;

  for (size_t num_threads = 1; num_threads <= kMaxThreads; num_threads++) {
    graph_store<> gs;
    mt_test(num_threads, [&](size_t thread_id) -> void {
      for (uint64_t i = 0; i < kNumNodes; i++) {
        for (uint64_t j = 1; j <= kDegree; j++) {
          node_op n1;
          n1.id = i;
          n1.type = 0;
          n1.data = std::to_string(i);
          uint64_t id1 = gs.add_node(n1);

          node_op n2;
          n2.id = (i + j) % kNumNodes;
          n2.type = 0;
          n2.data = std::to_string((i + j) % kNumNodes);
          uint64_t id2 = gs.add_node(n2);

          link_op l;
          l.id1 = id1;
          l.id2 = id2;
          l.link_type = 0;
          l.time = (i * kDegree + j);
          l.data = std::to_string(id1) + "->" + std::to_string(id2);
          bool success = gs.add_link(l);
          ASSERT_TRUE(success);
        }
      }
    });
  }
}

#include "graph_store.h"
#include "gtest/gtest.h"

#include <thread>

using namespace graphstore;
using namespace datastore;

class GraphStoreRSTest : public testing::Test {
 public:
  static const uint64_t kNumNodes = 1000;
  static const uint64_t kDegree = 10;
  static const uint64_t kNumLinks = 10000;
};

TEST_F(GraphStoreRSTest, AddNodeTest) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }
}

TEST_F(GraphStoreRSTest, GetNodeTest) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n = gs.get_node(i % 2, i);
    ASSERT_EQ(i, n.id);
    ASSERT_EQ(static_cast<int64_t>(i % 2), n.type);
    ASSERT_EQ(std::to_string(i), n.data);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n = gs.get_node((i + 1) % 2, i);
    ASSERT_EQ(UINT64_MAX, n.id);
    ASSERT_EQ(-1, n.type);
    ASSERT_EQ("", n.data);
  }

  for (uint64_t i = kNumNodes; i < 2 * kNumNodes; i++) {
    node_op n = gs.get_node(i % 2, i);
    ASSERT_EQ(UINT64_MAX, n.id);
    ASSERT_EQ(-1, n.type);
    ASSERT_EQ("", n.data);
  }
}

TEST_F(GraphStoreRSTest, UpdateNodeTest) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.data = std::to_string(kNumNodes + i);
    n.type = i % 2;
    bool success = gs.update_node(n);
    ASSERT_TRUE(success);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n = gs.get_node(i % 2, i);
    ASSERT_EQ(i, n.id);
    ASSERT_EQ(static_cast<int64_t>(i % 2), n.type);
    ASSERT_EQ(std::to_string(kNumNodes + i), n.data);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n = gs.get_node((i + 1) % 2, i);
    ASSERT_EQ(UINT64_MAX, n.id);
    ASSERT_EQ(-1, n.type);
    ASSERT_EQ("", n.data);
  }
}

TEST_F(GraphStoreRSTest, DeleteNodeTest) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    bool success = gs.delete_node(i % 2, i);
    ASSERT_TRUE(success);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n = gs.get_node(i % 2, i);
    ASSERT_EQ(UINT64_MAX, n.id);
    ASSERT_EQ(-1, n.type);
    ASSERT_EQ("", n.data);
  }
}

TEST_F(GraphStoreRSTest, AddGetLinkTest) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }

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

TEST_F(GraphStoreRSTest, UpdateGetLinkTest) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    for (uint64_t j = 1; j <= kDegree; j++) {
      link_op l;
      l.id1 = i;
      l.id2 = (i + j) % kNumNodes;
      l.link_type = j;
      l.time = kNumLinks - (i * kDegree + j);
      l.data = std::to_string(i) + "->" + std::to_string(j);
      bool success = gs.update_link(l);
      ASSERT_TRUE(success);
    }
  }

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

TEST_F(GraphStoreRSTest, AddUpdateGetLinkTest) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }

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

  for (uint64_t i = 0; i < kNumNodes; i++) {
    for (uint64_t j = 1; j <= kDegree; j++) {
      link_op l;
      l.id1 = i;
      l.id2 = (i + j) % kNumNodes;
      l.link_type = j;
      l.time = i * kDegree + j;
      l.data = std::to_string(i) + "=>" + std::to_string(j);
      bool success = gs.update_link(l);
      ASSERT_TRUE(success);
    }
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    for (uint64_t j = 1; j <= kDegree; j++) {
      link_op l = gs.get_link(i, j, (i + j) % kNumNodes);
      ASSERT_EQ(static_cast<int64_t>(i), l.id1);
      ASSERT_EQ(static_cast<int64_t>(j), l.link_type);
      ASSERT_EQ(static_cast<int64_t>((i + j) % kNumNodes), l.id2);
      ASSERT_EQ(static_cast<int64_t>(i * kDegree + j), l.time);
      ASSERT_EQ(std::to_string(i) + "=>" + std::to_string(j), l.data);
    }
  }
}

TEST_F(GraphStoreRSTest, AddDeleteLinkTest) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }

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

  for (uint64_t i = 0; i < kNumNodes; i++) {
    for (uint64_t j = 1; j <= kDegree; j++) {
      bool success = gs.delete_link(i, j, (i + j) % kNumNodes);
      ASSERT_TRUE(success);
    }
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    for (uint64_t j = 1; j <= kDegree; j++) {
      link_op l = gs.get_link(i, j, (i + j) % kNumNodes);
      ASSERT_EQ(-1LL, l.id1);
      ASSERT_EQ(-1LL, l.id2);
      ASSERT_EQ(-1LL, l.link_type);
      ASSERT_EQ(-1LL, l.time);
      ASSERT_EQ("", l.data);
    }
  }
}

TEST_F(GraphStoreRSTest, AddMultiGetLinkTest) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    for (uint64_t j = 1; j <= kDegree; j++) {
      link_op l;
      l.id1 = i;
      l.id2 = (i + j) % kNumNodes;
      l.link_type = l.id2 % 2;
      l.time = i;
      l.data = std::to_string(i);
      bool success = gs.add_link(l);
      ASSERT_TRUE(success);
    }
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    std::set<int64_t> id2s;
    for (uint64_t j = 1; j <= kDegree; j++)
      id2s.insert((i + j) % kNumNodes);

    {
      std::vector<link_op> links = gs.multiget_links(i, 0, id2s);
      ASSERT_EQ(kDegree / 2, links.size());
      for (link_op l : links) {
        ASSERT_EQ(static_cast<int64_t>(i), l.id1);
        ASSERT_EQ(0LL, l.id2 % 2);
        ASSERT_EQ(0LL, l.link_type);
        ASSERT_EQ(static_cast<int64_t>(i), l.time);
        ASSERT_EQ(std::to_string(i), l.data);
      }
    }

    {
      std::vector<link_op> links = gs.multiget_links(i, 1, id2s);
      ASSERT_EQ(kDegree / 2, links.size());
      for (link_op l : links) {
        ASSERT_EQ(static_cast<int64_t>(i), l.id1);
        ASSERT_EQ(1LL, l.id2 % 2);
        ASSERT_EQ(1LL, l.link_type);
        ASSERT_EQ(static_cast<int64_t>(i), l.time);
        ASSERT_EQ(std::to_string(i), l.data);
      }
    }

    for (uint64_t j = 3; j <= kDegree; j++)
      id2s.erase((i + j) % kNumNodes);

    {
      std::vector<link_op> links = gs.multiget_links(i, 0, id2s);
      ASSERT_EQ(1ULL, links.size());
      for (link_op l : links) {
        ASSERT_EQ(static_cast<int64_t>(i), l.id1);
        ASSERT_EQ(0LL, l.id2 % 2);
        ASSERT_EQ(0LL, l.link_type);
        ASSERT_EQ(static_cast<int64_t>(i), l.time);
        ASSERT_EQ(std::to_string(i), l.data);
      }
    }

    {
      std::vector<link_op> links = gs.multiget_links(i, 1, id2s);
      ASSERT_EQ(1ULL, links.size());
      for (link_op l : links) {
        ASSERT_EQ(static_cast<int64_t>(i), l.id1);
        ASSERT_EQ(1LL, l.id2 % 2);
        ASSERT_EQ(1LL, l.link_type);
        ASSERT_EQ(static_cast<int64_t>(i), l.time);
        ASSERT_EQ(std::to_string(i), l.data);
      }
    }
  }
}

TEST_F(GraphStoreRSTest, AddGetLinkList1Test) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    for (uint64_t j = 1; j <= kDegree; j++) {
      link_op l;
      l.id1 = i;
      l.id2 = (i + j) % kNumNodes;
      l.link_type = l.id2 % 2;
      l.time = i;
      l.data = std::to_string(i);
      bool success = gs.add_link(l);
      ASSERT_TRUE(success);
    }
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    {
      std::set<link_op> links = gs.get_link_list(i, 0);
      ASSERT_EQ(kDegree / 2, links.size());
      for (link_op l : links) {
        ASSERT_EQ(static_cast<int64_t>(i), l.id1);
        ASSERT_EQ(0LL, l.id2 % 2);
        ASSERT_EQ(0LL, l.link_type);
        ASSERT_EQ(static_cast<int64_t>(i), l.time);
        ASSERT_EQ(std::to_string(i), l.data);
      }
    }

    {
      std::set<link_op> links = gs.get_link_list(i, 1);
      ASSERT_EQ(kDegree / 2, links.size());
      for (link_op l : links) {
        ASSERT_EQ(static_cast<int64_t>(i), l.id1);
        ASSERT_EQ(1LL, l.id2 % 2);
        ASSERT_EQ(1LL, l.link_type);
        ASSERT_EQ(static_cast<int64_t>(i), l.time);
        ASSERT_EQ(std::to_string(i), l.data);
      }
    }
  }
}

TEST_F(GraphStoreRSTest, AddGetLinkList2Test) {
  graph_store<read_stalled_tail> gs;

  for (uint64_t i = 0; i < kNumNodes; i++) {
    node_op n;
    n.id = i;
    n.type = i % 2;
    n.data = std::to_string(i);
    uint64_t id = gs.add_node(n);
    ASSERT_EQ(i, id);
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    for (uint64_t j = 1; j <= kDegree; j++) {
      link_op l;
      l.id1 = i;
      l.id2 = (i + j) % kNumNodes;
      l.link_type = l.id2 % 2;
      l.time = j;
      l.data = std::to_string(i);
      bool success = gs.add_link(l);
      ASSERT_TRUE(success);
    }
  }

  for (uint64_t i = 0; i < kNumNodes; i++) {
    {
      std::set<link_op> links = gs.get_link_list(i, 0, 1, 6, 0, 10);
      ASSERT_EQ(3ULL, links.size());
      for (link_op l : links) {
        ASSERT_EQ(static_cast<int64_t>(i), l.id1);
        ASSERT_EQ(0LL, l.id2 % 2);
        ASSERT_EQ(0LL, l.link_type);
        ASSERT_TRUE(l.time >= 1 && l.time <= 6);
        ASSERT_EQ(std::to_string(i), l.data);
      }
    }

    {
      std::set<link_op> links = gs.get_link_list(i, 1, 1, 6, 0, 10);
      ASSERT_EQ(3ULL, links.size());
      for (link_op l : links) {
        ASSERT_EQ(static_cast<int64_t>(i), l.id1);
        ASSERT_EQ(1LL, l.id2 % 2);
        ASSERT_EQ(1LL, l.link_type);
        ASSERT_TRUE(l.time >= 1 && l.time <= 6);
        ASSERT_EQ(std::to_string(i), l.data);
      }
    }

    {
      std::set<link_op> links = gs.get_link_list(i, 1, 1, 6, 1, 1);
      ASSERT_EQ(1ULL, links.size());
      for (link_op l : links) {
        ASSERT_EQ(static_cast<int64_t>(i), l.id1);
        ASSERT_EQ(1LL, l.id2 % 2);
        ASSERT_EQ(1LL, l.link_type);
        ASSERT_TRUE(l.time >= 3 && l.time <= 4);
        ASSERT_EQ(std::to_string(i), l.data);
      }
    }
  }
}


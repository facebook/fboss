// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>
#include <memory>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include "common/base/Random.h"

#include "PyRadixWrapper.h"
#include "fboss/lib/RadixTree.h"

using namespace facebook;
using namespace facebook::network;
using namespace std;

namespace {
using IPAddress = folly::IPAddress;
using IPAddressV4 = folly::IPAddressV4;
using IPAddressV6 = folly::IPAddressV6;
template <typename IPAddrType>
static IPAddrType getIP(const radix_node_t& n);

template <>
IPAddressV4 getIP<IPAddressV4>(const radix_node_t& n) {
  if (n.prefix) {
    return IPAddressV4(n.prefix->add.sin);
  }
  CHECK(false);
  return IPAddressV4("0.0.0.0");
}

template <typename IPAddrType, typename T>
struct PyRadixNodeAccessor {
  static const radix_node_t* left(const radix_node_t& n) {
    return n.l;
  }
  static const radix_node_t* right(const radix_node_t& n) {
    return n.r;
  }
  static const T& value(const radix_node_t& n) {
    return **(static_cast<std::shared_ptr<T>*>(n.data));
  }

  static IPAddrType ipAddress(const radix_node_t& n) {
    return getIP<IPAddrType>(n);
  }

  static uint8_t masklen(const radix_node_t& n) {
    return n.bit;
  }

  static bool isNonValueNode(const radix_node_t& n) {
    return n.prefix == nullptr;
  }
};

// Code to compare our radix tree and py-radix tree
template <typename IPAddrType, typename T>
struct RadixTreeNodeAccessor {
  typedef RadixTreeNode<IPAddrType, T> TreeNode;
  static IPAddrType ipAddress(const TreeNode& node) {
    return node.ipAddress();
  }
  static uint8_t masklen(const TreeNode& node) {
    return node.masklen();
  }
  static bool isNonValueNode(const TreeNode& node) {
    return node.isNonValueNode();
  }
  static const T& value(const TreeNode& node) {
    if (!node.isValueNode()) {
      throw logic_error("Can't access value on non value nodes");
    }
    return node.value();
  }
  static const TreeNode* left(const TreeNode& node) {
    return node.left();
  }
  static const TreeNode* right(const TreeNode& node) {
    return node.right();
  }
};

template <typename IPAddrType, typename T>
struct RadixAndPyRadixNodeEqual {
  bool operator()(const RadixTreeNode<IPAddrType, T>& l, const radix_node_t& r)
      const {
    if (RadixTreeNodeAccessor<IPAddrType, T>::isNonValueNode(l) &&
        PyRadixNodeAccessor<IPAddrType, T>::isNonValueNode(r)) {
      return RadixTreeNodeAccessor<IPAddrType, T>::masklen(l) ==
          PyRadixNodeAccessor<IPAddrType, T>::masklen(r);
    }
    return (
        RadixTreeNodeAccessor<IPAddrType, T>::ipAddress(l) ==
            PyRadixNodeAccessor<IPAddrType, T>::ipAddress(r) &&
        RadixTreeNodeAccessor<IPAddrType, T>::masklen(l) ==
            PyRadixNodeAccessor<IPAddrType, T>::masklen(r) &&
        RadixTreeNodeAccessor<IPAddrType, T>::isNonValueNode(l) ==
            PyRadixNodeAccessor<IPAddrType, T>::isNonValueNode(r) &&
        // Compare values on non internal nodes
        (RadixTreeNodeAccessor<IPAddrType, T>::isNonValueNode(l) ||
         RadixTreeNodeAccessor<IPAddrType, T>::value(l) ==
             PyRadixNodeAccessor<IPAddrType, T>::value(r)));
  }
};

/*
 * Compare any 2 radix tree nodes with links (left, right, parent) ignored
 * This is templatized for 2 different types to allow us to compare radix
 * tree nodes of 2 different radix tree implementation (as long as both
 * nodes satisfy the interface).
 */
template <
    typename IPAddrType,
    typename T,
    typename NodeA,
    typename NodeB,
    typename NodeAAccessor = RadixTreeNodeAccessor<IPAddrType, T>,
    typename NodeBAccessor = RadixTreeNodeAccessor<IPAddrType, T>>
struct RadixTreeNodeEqualSansLinks {
  bool operator()(const NodeA& nodeA, const NodeB& nodeB) const {
    return (
        NodeAAccessor::ipAddress(nodeA) == NodeBAccessor::ipAddress(nodeB) &&
        NodeAAccessor::masklen(nodeA) == NodeBAccessor::masklen(nodeB) &&
        NodeAAccessor::isNonValueNode(nodeA) ==
            NodeBAccessor::isNonValueNode(nodeB) &&
        // Compare values of nodes that contain values
        (NodeAAccessor::isNonValueNode(nodeA) ||
         NodeAAccessor::value(nodeA) == NodeBAccessor::value(nodeB)));
  }
};

// Compare 2 (sub) radix trees
template <
    typename IPAddrType,
    typename T,
    typename NodeA,
    typename NodeB,
    typename NodeAAccessor = RadixTreeNodeAccessor<IPAddrType, T>,
    typename NodeBAccessor = RadixTreeNodeAccessor<IPAddrType, T>,
    typename NodeEqual = RadixTreeNodeEqualSansLinks<
        T,
        NodeA,
        NodeB,
        NodeAAccessor,
        NodeBAccessor>>
bool radixTreeEqual(const NodeA* nodeA, const NodeB* nodeB) {
  if (nodeA && nodeB) {
    if (NodeEqual()(*nodeA, *nodeB)) {
      return radixTreeEqual<
                 IPAddrType,
                 T,
                 NodeA,
                 NodeB,
                 NodeAAccessor,
                 NodeBAccessor,
                 NodeEqual>(
                 NodeAAccessor::left(*nodeA), NodeBAccessor::left(*nodeB)) &&
          radixTreeEqual<
                 IPAddrType,
                 T,
                 NodeA,
                 NodeB,
                 NodeAAccessor,
                 NodeBAccessor,
                 NodeEqual>(
                 NodeAAccessor::right(*nodeA), NodeBAccessor::right(*nodeB));
    } else {
      return false;
    }
  }
  return (!nodeA && !nodeB);
}

IPAddressV4 ip0_0_0_0("0.0.0.0");
IPAddressV4 ip48_0_0_0("48.0.0.0");
IPAddressV4 ip64_0_0_0("64.0.0.0");
IPAddressV4 ip72_0_0_0("72.0.0.0");
IPAddressV4 ip80_0_0_0("80.0.0.0");
IPAddressV4 ip80_0_0_1("80.0.0.1");
IPAddressV4 ip128_0_0_0("128.0.0.0");
IPAddressV4 ip160_0_0_0("160.0.0.0");

// Same addresses as above but now in IPV6
IPAddressV6 ip6_0("::");
IPAddressV6 ip6_48("3000::");
IPAddressV6 ip6_64("4000::");
IPAddressV6 ip6_72("4800::");
IPAddressV6 ip6_80("5000::");
IPAddressV6 ip6_80_1("5000::1");
IPAddressV6 ip6_128("8000::");
IPAddressV6 ip6_160("A000::");

vector<Prefix4> setupTestTree4(RadixTree<IPAddressV4, int>& rtree) {
  vector<Prefix4> inserted;
  // First node insert 128/2 should become the root
  rtree.insert(ip128_0_0_0, 2, 1);
  inserted.push_back({ip128_0_0_0, 2});
  // Insert 128/1
  // Tests 2 things
  // - 128/1 should become the new root. So this tests a to be inserted
  //   node becoming root.
  // - Tests inserting a ip, for which the same ip with a different mask
  //   already exists in the tree
  rtree.insert(ip128_0_0_0, 1, 2);
  inserted.push_back({ip128_0_0_0, 1});
  // Inserting 80/4. Tests creation of a new non value node as root.
  // 0/0 should become the new root
  rtree.insert(ip80_0_0_0, 4, 3);
  inserted.push_back({ip80_0_0_0, 4});
  // Test inserting a node which becomes a parent of a existing node.
  // Here 64/3 should be inserted as a parent of 80/4, which should be
  // the right child of 64/3. Tests insertion of a node b/w 2 existing
  // nodes. Test the code path where we did not need to construct a
  // extra internal node for accommodating 64/3.
  rtree.insert(ip64_0_0_0, 3, 4);
  inserted.push_back({ip64_0_0_0, 3});
  // 160/3 should become the right child of 128/2. Tests adding a leaf
  rtree.insert(ip160_0_0_0, 3, 5);
  /// 0/1 should become the left child of root (0/0). Tests adding a
  // node b/w 2 existing nodes, again w/o needing to create a extra
  // internal node.
  rtree.insert(ip0_0_0_0, 1, 6);
  inserted.push_back({ip0_0_0_0, 1});
  // 72/6 should become the left child of 64/3. Tests adding a leaf
  rtree.insert(ip72_0_0_0, 6, 7);
  inserted.push_back({ip72_0_0_0, 6});
  // 48/5 should become the left child of 0/1. Tests adding a leaf
  rtree.insert(ip48_0_0_0, 5, 8);
  // 0/4 should cause creation of a internal node 0/2 as a left
  // child of 0/1 which should then have 0/4 and 48/5 as its left
  // and right children. Tests creation of new internal node, and
  // linking it in the tree.
  rtree.insert(ip0_0_0_0, 4, 9);
  inserted.push_back({ip0_0_0_0, 4});
  /*
   * Thus the tree should look like so
   * - All prefixes have 0's in the last 3 octets
   * - Parenthesis contain value stored on node or * for non value nodes
   *                      0/0 (*)
   *                    /         \
   *                   /           \
   *            0/1(6)              128/1(2)
   *           /     \              /
   *          /       \            128/2(1)
   *      0/2(*)      64/3(4)             \
   *     /     \      /     \             160/3(5)
   * 0/4(9) 48/5(8)  72/6(7) 80/4(3)
   */
  return inserted;
}

vector<Prefix6> setupTestTree6(RadixTree<IPAddressV6, int>& rtree) {
  vector<Prefix6> inserted;
  // First node insert 8000::/2 should become the root
  rtree.insert(ip6_128, 2, 1);
  inserted.push_back({ip6_128, 2});
  // Insert 8000::/1
  // Tests 2 things
  // - 8000::/1 should become the new root. So this tests a to be inserted
  //   node becoming root.
  // - Tests inserting a ip, for which the same ip with a different mask
  //   already exists in the tree
  rtree.insert(ip6_128, 1, 2);
  inserted.push_back({ip6_128, 1});
  /// Inserting 5000::/4. Tests creation of a new non value node as root.
  // ::/0 should become the new root
  rtree.insert(ip6_80, 4, 3);
  inserted.push_back({ip6_80, 4});
  // Test inserting a node which becomes a parent of a existing node.
  // Here 4000::/3 should be inserted as a parent of 5000::/4, which should be
  // the right child of 4000::/3. Tests insertion of a node b/w 2 existing
  // nodes. Test the code path where we did not need to construct a
  // extra internal node for accommodating 4000::/3.
  rtree.insert(ip6_64, 3, 4);
  inserted.push_back({ip6_64, 3});
  // a000::/3 should become the right child of 8000::/2. Tests adding a leaf
  rtree.insert(ip6_160, 3, 5);
  // ::/1 should become the left child of root (::/0). Tests adding a
  // node b/w 2 existing nodes, again w/o needing to create a extra
  // internal node.
  rtree.insert(ip6_0, 1, 6);
  inserted.push_back({ip6_0, 1});
  // 4800::/6 should become the left child of 4000::/3. Tests adding a leaf
  rtree.insert(ip6_72, 6, 7);
  inserted.push_back({ip6_72, 6});
  // 3000::/5 should become the left child of ::/1. Tests adding a leaf
  rtree.insert(ip6_48, 5, 8);
  // ::/4 should cause creation of a internal node ::/2 as a left
  // child of ::/1 which should then have ::/4 and 3000::/5 as its left
  // and right children. Tests creation of new internal node, and
  // linking it in the tree.
  rtree.insert(ip6_0, 4, 9);
  inserted.push_back({ip6_0, 4});
  /* Thus the tree should look like so
   * - All prefixes have 0's in the last 3 octets
   * - Parenthesis contain value stored on node or * for non value nodes
   *                        ::/0(*)
   *                    /            \
   *                   /              \
   *             ::/1(6)              800::/1(2)
   *           /        \              /
   *          /          \            800::/2(1)
   *      ::/2(*)         400::/3(4)             \
   *     /     \          /     \             a00::/3(5)
   * ::/4(9) 300::/5(8) 480::/6(7) 500::/4(3)
   *
   */
  return inserted;
}

} // namespace

TEST(RadixTree, Erase4) {
  // Erase has the following code paths
  // 0) Try to erase  a non value/non existing node. Tree should be unchanged.
  // 1) Erase node with 2 children.
  // 2) Erase node with 1 child.
  // 3) Erase node with 1 child, where node is root of the tree.
  // 4) Erase node with no children where node's parent is a non value node
  //    and thus would also need to be deleted.
  // 5) Erase node with no children where node's parent is a value node and
  //    and thus does node need to be deleted.
  // 6) Erase node with no children where node is root of the tree.

  // The following tests exercise all these code paths and then some.
  auto deleteCount = 0;
  auto deleteCallback = [&](const RadixTreeNode<IPAddressV4, int>& /*node*/) {
    ++deleteCount;
  };
  RadixTree<IPAddressV4, int> rtree(deleteCallback), rtreeOrig;
  typedef RadixTreeNode<IPAddressV4, int> IterValue;
  auto counter = [](int cnt, const IterValue& /*i*/) { return cnt + 1; };
  setupTestTree4(rtree);
  setupTestTree4(rtreeOrig);
  // Trees are identical
  EXPECT_TRUE(rtree == rtreeOrig);
  RadixTree<IPAddressV4, int>::VecConstIterators trail;
  // Test 0 - Can't delete a node that you didn't insert
  // tree unchanged.
  auto beforeDeleteCount = deleteCount;
  EXPECT_FALSE(rtree.erase(ip0_0_0_0, 0));
  EXPECT_EQ(beforeDeleteCount, deleteCount);
  EXPECT_TRUE(rtree == rtreeOrig);
  EXPECT_EQ(rtree.size(), accumulate(rtree.begin(), rtree.end(), 0, counter));
  // Test 1 - Delete a node with 2 children. This
  // should just transform the node to a non value node
  // Lookup a node further down from the about to be erase node
  // and compare the trails before and after. The only
  // difference should be in the values held on the nodes
  // otherwise the trails should be the same.
  auto matchItr = rtree.exactMatchWithTrail(
      ip48_0_0_0, 5, trail, true /* include non value nodes*/);
  auto sizeBefore = rtree.size();
  auto expectedTrail = trailStr(trail, false /*don't print values*/);
  EXPECT_TRUE(!matchItr.atEnd());
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip0_0_0_0, 1));
  /*  Tree should now be
   *
   *                       0/0 (*)
   *                    /         \
   *                   /           \
   *            0/1(*)              128/1(2)
   *           /     \              /
   *          /       \            128/2(1)
   *      0/2(*)      64/3(4)             \
   *     /     \      /     \             160/3(5)
   * 0/4(9) 48/5(8)  72/6(7) 80/4(3)
   */
  EXPECT_EQ(beforeDeleteCount, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  // Trees diverged
  EXPECT_FALSE(rtree == rtreeOrig);
  trail.clear();
  rtree.exactMatchWithTrail(
      ip48_0_0_0, 5, trail, true /*include non value nodes*/);

  // Try searching 0/3, this should go till 0/4 and then retreat
  // to the last non value node, which should be NULL.
  EXPECT_EQ(rtree.end(), rtree.longestMatch(IPAddressV4("0.0.0.0"), 3));
  // Find 64/5, this should get to 72/6 and the retreat back to the
  // last value node seen, which should be 64/3
  auto node = rtree.longestMatch(ip64_0_0_0, 7);
  EXPECT_EQ(ip64_0_0_0, node->ipAddress());
  EXPECT_EQ(3, node->masklen());
  EXPECT_EQ(expectedTrail, trailStr(trail, false /*don't print values*/));
  // Test 2 - Delete a node with 1 child. It does not really matter
  // whether the parent is a value node or not, but we test both.
  // Here parent is a value node.
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip128_0_0_0, 2));
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  EXPECT_EQ(beforeDeleteCount + 1, deleteCount);
  EXPECT_FALSE(rtree == rtreeOrig);
  /*  Tree should now be
   *
   *                       0/0 (*)
   *                    /         \
   *                   /           \
   *            0/1(*)              128/1(2)
   *           /     \              /
   *          /       \            160/3(5)
   *      0/2(*)      64/3(4)
   *     /     \      /     \
   * 0/4(9) 48/5(8)  72/6(7) 80/4(3)
   */

  rtree.exactMatchWithTrail(
      ip160_0_0_0, 3, trail, true /*include non value nodes*/);
  expectedTrail = "0.0.0.0/0(*), 128.0.0.0/1(2), 160.0.0.0/3(5)";
  EXPECT_EQ(expectedTrail, trailStr(trail));

  // Test 3 - Delete a node with one child, parent is non value node
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip128_0_0_0, 1));
  /*  Tree should now be
   *
   *                       0/0 (*)
   *                    /         \
   *                   /           \
   *            0/1(*)              160/3(5)
   *           /     \
   *          /       \
   *      0/2(*)      64/3(4)
   *     /     \      /     \
   * 0/4(9) 48/5(8)  72/6(7) 80/4(3)
   */
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  EXPECT_EQ(beforeDeleteCount + 1, deleteCount);
  rtree.exactMatchWithTrail(
      ip160_0_0_0, 3, trail, true /*include non value nodes*/);
  EXPECT_FALSE(rtree == rtreeOrig);
  expectedTrail = "0.0.0.0/0(*), 160.0.0.0/3(5)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  // Test 4 - Delete a node with no children whose parent is a non value node.
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip48_0_0_0, 5));
  /*  Tree should now be
   *
   *                       0/0 (*)
   *                    /         \
   *                   /           \
   *            0/1(*)              160/3(5)
   *           /     \
   *          /       \
   *      0/4(9)      64/3(4)
   *                  /     \
   *               72/6(7) 80/4(3)
   */
  EXPECT_EQ(beforeDeleteCount + 2, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  rtree.exactMatchWithTrail(
      ip0_0_0_0, 4, trail, true /*include non value nodes*/);
  expectedTrail = "0.0.0.0/0(*), 0.0.0.0/1(*), 0.0.0.0/4(9)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_FALSE(rtree == rtreeOrig);
  // Test 5 - Delete a node with no children whose parent is a non
  // value node and is root of the tree
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip160_0_0_0, 3));
  /*  Tree should now be
   *            0/1(*)
   *           /     \
   *          /       \
   *      0/4(9)      64/3(4)
   *                  /     \
   *               72/6(7) 80/4(3)
   */
  EXPECT_EQ(beforeDeleteCount + 2, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  rtree.exactMatchWithTrail(
      ip72_0_0_0, 6, trail, true /*include non value nodes*/);
  expectedTrail = "0.0.0.0/1(*), 64.0.0.0/3(4), 72.0.0.0/6(7)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_FALSE(rtree == rtreeOrig);
  // Test 6 - Delete a node with no children whose parent is a
  // value node
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip80_0_0_0, 4));
  /*  Tree should now be
   *            0/1(*)
   *           /     \
   *          /       \
   *      0/4(9)      64/3(4)
   *                  /
   *               72/6(7)
   */
  EXPECT_EQ(beforeDeleteCount + 1, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  rtree.exactMatchWithTrail(
      ip72_0_0_0, 6, trail, true /*include non value nodes*/);
  expectedTrail = "0.0.0.0/1(*), 64.0.0.0/3(4), 72.0.0.0/6(7)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_FALSE(rtree == rtreeOrig);

  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip0_0_0_0, 4));
  /* Tree should now be
   *    64/3(4)
   *     /
   * 72/6(7)
   */
  EXPECT_EQ(beforeDeleteCount + 2, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  EXPECT_FALSE(rtree == rtreeOrig);
  // Test 7 - Delete a node with 1 child with this node being root
  // of the tree.
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip64_0_0_0, 3));
  /*  Tree should now be
   *  72/6(7)
   */
  EXPECT_EQ(beforeDeleteCount + 1, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  rtree.exactMatchWithTrail(
      ip72_0_0_0, 6, trail, true /*include non value nodes*/);
  expectedTrail = "72.0.0.0/6(7)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_FALSE(rtree == rtreeOrig);
  // Test 8 - Delete a node with no children which is also the root
  // of the tree
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip72_0_0_0, 6));
  /*  Tree should now be
   *   nullptr
   */
  EXPECT_EQ(beforeDeleteCount + 1, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(0, rtree.size());
  EXPECT_EQ(0, accumulate(rtree.begin(), rtree.end(), 0, counter));
  EXPECT_FALSE(rtree == rtreeOrig);
  EXPECT_EQ(nullptr, rtree.root());
}

TEST(RadixTree, Inserts4) {
  auto valueNodesDeleted = 0;
  auto valueNodeDeleteCounter = [&](const RadixTreeNode<IPAddressV4, int>& n) {
    valueNodesDeleted += !n.isNonValueNode();
  };
  RadixTree<IPAddressV4, int> rtree(valueNodeDeleteCounter);
  auto prefixesInserted = setupTestTree4(rtree);
  RadixTree<IPAddressV4, int>::VecConstIterators trail;
  auto matchItr = rtree.exactMatchWithTrail(ip128_0_0_0, 2, trail, true);
  auto expectedTrail = "0.0.0.0/0(*), 128.0.0.0/1(2), 128.0.0.0/2(1)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  matchItr = rtree.exactMatchWithTrail(ip160_0_0_0, 3, trail, true);
  expectedTrail =
      "0.0.0.0/0(*), 128.0.0.0/1(2), 128.0.0.0/2(1), "
      "160.0.0.0/3(5)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  matchItr = rtree.exactMatchWithTrail(ip0_0_0_0, 4, trail, true);
  expectedTrail =
      "0.0.0.0/0(*), 0.0.0.0/1(6), 0.0.0.0/2(*), "
      "0.0.0.0/4(9)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  matchItr = rtree.exactMatchWithTrail(ip48_0_0_0, 5, trail, true);
  expectedTrail =
      "0.0.0.0/0(*), 0.0.0.0/1(6), 0.0.0.0/2(*), "
      "48.0.0.0/5(8)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  matchItr = rtree.exactMatchWithTrail(ip72_0_0_0, 6, trail, true);
  expectedTrail = "0.0.0.0/0(*), 0.0.0.0/1(6), 64.0.0.0/3(4), 72.0.0.0/6(7)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  matchItr = rtree.exactMatchWithTrail(ip80_0_0_0, 4, trail, true);
  expectedTrail = "0.0.0.0/0(*), 0.0.0.0/1(6), 64.0.0.0/3(4), 80.0.0.0/4(3)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  // exact match should succeed, even if IPAddress is not properly masked
  matchItr = rtree.exactMatchWithTrail(ip80_0_0_1, 4, trail, true);
  expectedTrail = "0.0.0.0/0(*), 0.0.0.0/1(6), 64.0.0.0/3(4), 80.0.0.0/4(3)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  // clear the tree
  auto sizeBefore = rtree.size();
  rtree.clear();
  EXPECT_EQ(sizeBefore, valueNodesDeleted);
  EXPECT_EQ(0, rtree.size());
  EXPECT_EQ(nullptr, rtree.root());
}

TEST(RadixTree, Iterator4) {
  RadixTree<IPAddressV4, int> rtree;
  auto prefixesInserted = setupTestTree4(rtree);
  // Verify that we get the same nodes regardless of whether we
  // iterate using a iterator or const iterator.
  const auto& crtree = rtree;
  typedef RadixTreeNode<IPAddressV4, int> IterValue;
  auto counter = [](int cnt, const IterValue& /*i*/) { return cnt + 1; };

  EXPECT_EQ(rtree.size(), accumulate(rtree.begin(), rtree.end(), 0, counter));
  EXPECT_EQ(
      crtree.size(), accumulate(crtree.begin(), crtree.end(), 0, counter));
  auto itr = rtree.begin();
  auto citr = crtree.begin();
  for (; itr != rtree.end() && citr != crtree.end(); itr++, citr++) {
    EXPECT_EQ(itr->value(), (*citr).value());
  }
  EXPECT_EQ(itr, rtree.end());
  EXPECT_EQ(citr, crtree.end());
  // Verify that iterator found via exact match is the same
  // as if we had encountered the node while traversing the tree.
  for (auto pfx : prefixesInserted) {
    auto iitr = rtree.exactMatch(pfx.ip, pfx.mask);
    auto jitr = rtree.begin();
    for (; jitr != rtree.end() && jitr != iitr; ++jitr)
      ;
    EXPECT_EQ(iitr, jitr);
    for (; iitr != rtree.end() && jitr != rtree.end(); ++iitr, ++jitr) {
      EXPECT_EQ(iitr, jitr);
    }
    EXPECT_EQ(rtree.end(), iitr);
    EXPECT_EQ(rtree.end(), jitr);
  }
  typedef RadixTreeIterator<IPAddressV4, int> IterType;
  typedef RadixTreeConstIterator<IPAddressV4, int> CIterType;
  EXPECT_EQ(IterType(), rtree.end());
  EXPECT_EQ(CIterType(), crtree.end());
}

/*
 * V6 tests, these mirror the V4 tests above, with V4 prefixes
 * from above translated to V6
 */

TEST(RadixTree, Erase6) {
  // Erase has the following code paths
  // 0) Try to erase  a non value/non existing node. Tree should be unchanged.
  // 1) Erase node with 2 children.
  // 2) Erase node with 1 child.
  // 3) Erase node with 1 child, where node is root of the tree.
  // 4) Erase node with no children where node's parent is a non value node
  //    and thus would also need to be deleted.
  // 5) Erase node with no children where node's parent is a value node
  //    and thus does node need to be deleted.
  // 6) Erase node with no children where node is root of the tree.

  // The following tests exercise all these code paths and then some.
  auto deleteCount = 0;
  auto deleteCallback = [&](const RadixTreeNode<IPAddressV6, int>& /*node*/) {
    ++deleteCount;
  };
  RadixTree<IPAddressV6, int> rtree(deleteCallback), rtreeOrig;
  typedef RadixTreeNode<IPAddressV6, int> IterValue;
  auto counter = [](int cnt, const IterValue& /*i*/) { return cnt + 1; };
  setupTestTree6(rtree);
  setupTestTree6(rtreeOrig);
  // Trees are identical
  EXPECT_TRUE(rtree == rtreeOrig);
  RadixTree<IPAddressV6, int>::VecConstIterators trail;

  // Test 0 - Can't delete a node that you didn't insert
  // tree unchanged.
  auto beforeDeleteCount = deleteCount;
  EXPECT_FALSE(rtree.erase(ip6_0, 0));
  EXPECT_EQ(beforeDeleteCount, deleteCount);
  EXPECT_TRUE(rtree == rtreeOrig);
  EXPECT_EQ(rtree.size(), accumulate(rtree.begin(), rtree.end(), 0, counter));
  // Test 1 - Delete a node with 2 children. This
  // should just transform the node to a non value node
  // Lookup a node further down from the about to be erase node
  // and compare the trails before and after. The only
  // difference should be in the values held on the nodes
  // otherwise the trails should be the same.
  auto matchItr = rtree.exactMatchWithTrail(
      ip6_48, 5, trail, true /* include non value nodes*/);
  auto sizeBefore = rtree.size();
  auto expectedTrail = trailStr(trail, false /*don't print values*/);
  EXPECT_NE(matchItr, rtree.end());
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip6_0, 1));
  /*
   *  Tree should now be
   *
   *                        ::/0 (*)
   *                    /                \
   *                   /                  \
   *            ::/1(*)                    8000::/1(2)
   *           /       \                        /
   *          /         \                     8000::/2(1)
   *      ::/2(*)        4000::/3(4)             \
   *     /     \            /        \           a000::/3(5)
   * ::/4(9) 3000::/5(8)  4800::/6(7) 5000::/4(3)
   */
  EXPECT_EQ(beforeDeleteCount, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  // Trees diverged
  EXPECT_FALSE(rtree == rtreeOrig);
  trail.clear();
  rtree.exactMatchWithTrail(ip6_48, 5, trail, true /*include non value nodes*/);
  EXPECT_EQ(expectedTrail, trailStr(trail, false /*don't print values*/));

  // Try searching ::/3, this should go till ::/4 and then retreat
  // to the last non value node, which should be NULL.
  EXPECT_EQ(rtree.longestMatch(IPAddressV6("::"), 3), rtree.end());
  // Find 4000::/5, this should get to 4800::/6 and the retreat back to the
  // last value node seen, which should be 4000::/3
  auto node = rtree.longestMatch(ip6_64, 7);
  EXPECT_EQ(ip6_64, node->ipAddress());
  EXPECT_EQ(3, node->masklen());

  // Test 2 - Delete a node with 1 child. It does not really matter
  // whether the parent is a value node or not, but we test both.
  // Here parent is a value node.
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip6_128, 2));
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  EXPECT_EQ(beforeDeleteCount + 1, deleteCount);
  EXPECT_FALSE(rtree == rtreeOrig);
  /*
   *  Tree should now be
   *
   *                       ::/0 (*)
   *                    /         \
   *                   /           \
   *            ::/1(*)              8000::/1(2)
   *           /     \              /
   *          /       \            160/3(5)
   *      0/2(*)      4000::/3(4)
   *     /     \      /     \
   * ::/4(9) 3000::/5(8)  4800::/6(7) 8::/4(3)
   */

  rtree.exactMatchWithTrail(
      ip6_160, 3, trail, true /*include non value nodes*/);
  expectedTrail = "::/0(*), 8000::/1(2), a000::/3(5)";
  EXPECT_EQ(expectedTrail, trailStr(trail));

  // Test 3 - Delete a node with one child, parent is non value node
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip6_128, 1));
  /*
   *  Tree should now be
   *
   *                       ::/0 (*)
   *                    /         \
   *                   /           \
   *            ::/1(*)              160/3(5)
   *           /     \
   *          /       \
   *      0/2(*)      4000::/3(4)
   *     /     \      /     \
   * ::/4(9) 3000::/5(8)  4800::/6(7) 8::/4(3)
   */
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  EXPECT_EQ(beforeDeleteCount + 1, deleteCount);
  rtree.exactMatchWithTrail(
      ip6_160, 3, trail, true /*include non value nodes*/);
  EXPECT_FALSE(rtree == rtreeOrig);
  expectedTrail = "::/0(*), a000::/3(5)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  // Test 4 - Delete a node with no children whose parent is a non
  // value node.
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip6_48, 5));
  /*
   *  Tree should now be
   *
   *                       ::/0 (*)
   *                    /         \
   *                   /           \
   *            ::/1(*)              160/3(5)
   *           /     \
   *          /       \
   *      ::/4(9)      4000::/3(4)
   *                  /     \
   *               4800::/6(7) 8::/4(3)
   */
  EXPECT_EQ(beforeDeleteCount + 2, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  rtree.exactMatchWithTrail(ip6_0, 4, trail, true /*include non value nodes*/);
  expectedTrail = "::/0(*), ::/1(*), ::/4(9)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_FALSE(rtree == rtreeOrig);
  // Test 5 - Delete a node with no children whose parent is a non
  // value node and is root of the tree
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip6_160, 3));
  /*
   *  Tree should now be
   *            ::/1(*)
   *           /     \
   *          /       \
   *      ::/4(9)      4000::/3(4)
   *                  /     \
   *               4800::/6(7) 8::/4(3)
   */
  EXPECT_EQ(beforeDeleteCount + 2, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  rtree.exactMatchWithTrail(ip6_72, 6, trail, true /*include non value nodes*/);
  expectedTrail = "::/1(*), 4000::/3(4), 4800::/6(7)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_FALSE(rtree == rtreeOrig);
  // Test 6 - Delete a node with no children whose parent is a
  // value node
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip6_80, 4));
  /*
   *  Tree should now be
   *            ::/1(*)
   *           /     \
   *          /       \
   *      ::/4(9)      4000::/3(4)
   *                  /
   *               4800::/6(7)
   */
  EXPECT_EQ(beforeDeleteCount + 1, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  rtree.exactMatchWithTrail(ip6_72, 6, trail, true /*include non value nodes*/);
  expectedTrail = "::/1(*), 4000::/3(4), 4800::/6(7)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_FALSE(rtree == rtreeOrig);

  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip6_0, 4));
  /*
   * Tree should now be
   *    4000::/3(4)
   *     /
   * 4800::/6(7)
   */
  EXPECT_EQ(beforeDeleteCount + 2, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  EXPECT_FALSE(rtree == rtreeOrig);
  // Test 7 - Delete a node with 1 child with this node being root
  // of the tree.
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip6_64, 3));
  //  Tree should now be
  //  4800::/6(7)
  EXPECT_EQ(beforeDeleteCount + 1, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(sizeBefore - 1, accumulate(rtree.begin(), rtree.end(), 0, counter));
  rtree.exactMatchWithTrail(ip6_72, 6, trail, true /*include non value nodes*/);
  expectedTrail = "4800::/6(7)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_FALSE(rtree == rtreeOrig);
  // Test 8 - Delete a node with no children which is also the root
  // of the tree
  trail.clear();
  sizeBefore = rtree.size();
  beforeDeleteCount = deleteCount;
  EXPECT_TRUE(rtree.erase(ip6_72, 6));
  /*
   *  Tree should now be
   *   nullptr
   */
  EXPECT_EQ(beforeDeleteCount + 1, deleteCount);
  EXPECT_EQ(sizeBefore - 1, rtree.size());
  EXPECT_EQ(0, rtree.size());
  EXPECT_EQ(0, accumulate(rtree.begin(), rtree.end(), 0, counter));
  EXPECT_FALSE(rtree == rtreeOrig);
  EXPECT_EQ(nullptr, rtree.root());
}

TEST(RadixTree, Inserts6) {
  auto valueNodesDeleted = 0;
  auto valueNodeDeleteCounter = [&](const RadixTreeNode<IPAddressV6, int>& n) {
    valueNodesDeleted += !n.isNonValueNode();
  };
  RadixTree<IPAddressV6, int> rtree(valueNodeDeleteCounter);
  auto prefixesInserted = setupTestTree6(rtree);
  RadixTree<IPAddressV6, int>::VecConstIterators trail;
  auto matchItr = rtree.exactMatchWithTrail(ip6_128, 2, trail, true);
  auto expectedTrail = "::/0(*), 8000::/1(2), 8000::/2(1)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();
  matchItr = rtree.exactMatchWithTrail(ip6_160, 3, trail, true);
  expectedTrail =
      "::/0(*), 8000::/1(2), 8000::/2(1), "
      "a000::/3(5)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();
  matchItr = rtree.exactMatchWithTrail(ip6_0, 4, trail, true);
  expectedTrail =
      "::/0(*), ::/1(6), ::/2(*), "
      "::/4(9)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  matchItr = rtree.exactMatchWithTrail(ip6_48, 5, trail, true);
  expectedTrail =
      "::/0(*), ::/1(6), ::/2(*), "
      "3000::/5(8)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  matchItr = rtree.exactMatchWithTrail(ip6_72, 6, trail, true);
  expectedTrail = "::/0(*), ::/1(6), 4000::/3(4), 4800::/6(7)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  matchItr = rtree.exactMatchWithTrail(ip6_80, 4, trail, true);
  expectedTrail = "::/0(*), ::/1(6), 4000::/3(4), 5000::/4(3)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  // Exact match should still succeed if IPAddress is not masked correctly
  matchItr = rtree.exactMatchWithTrail(ip6_80_1, 4, trail, true);
  expectedTrail = "::/0(*), ::/1(6), 4000::/3(4), 5000::/4(3)";
  EXPECT_EQ(expectedTrail, trailStr(trail));
  EXPECT_EQ(expectedTrail, trailStr(pathFromRoot(matchItr, true)));
  trail.clear();

  // clear the tree
  auto sizeBefore = rtree.size();
  rtree.clear();
  EXPECT_EQ(sizeBefore, valueNodesDeleted);
  EXPECT_EQ(0, rtree.size());
  EXPECT_EQ(nullptr, rtree.root());
}

TEST(RadixTree, Iterator6) {
  RadixTree<IPAddressV6, int> rtree;
  auto prefixesInserted = setupTestTree6(rtree);
  // Verify that we get the same nodes regardless of whether we
  // iterate using a iterator or const iterator.
  const auto& crtree = rtree;
  typedef RadixTreeNode<IPAddressV6, int> IterValue;
  auto counter = [](int cnt, const IterValue& /*i*/) { return cnt + 1; };

  EXPECT_EQ(rtree.size(), accumulate(rtree.begin(), rtree.end(), 0, counter));
  EXPECT_EQ(
      crtree.size(), accumulate(crtree.begin(), crtree.end(), 0, counter));
  auto itr = rtree.begin();
  auto citr = crtree.begin();
  for (; itr != rtree.end() && citr != crtree.end(); itr++, citr++) {
    EXPECT_EQ(itr->value(), (*citr).value());
  }
  EXPECT_EQ(itr, rtree.end());
  EXPECT_EQ(citr, crtree.end());
  // Verify that iterator found via exact match is the same
  // as if we had encountered the node while traversing the tree.
  for (auto pfx : prefixesInserted) {
    auto iitr = rtree.exactMatch(pfx.ip, pfx.mask);
    auto jitr = rtree.begin();
    for (; jitr != rtree.end() && jitr != iitr; ++jitr)
      ;
    EXPECT_EQ(iitr, jitr);
    for (; iitr != rtree.end() && jitr != rtree.end(); ++iitr, ++jitr) {
      EXPECT_EQ(iitr, jitr);
    }
    EXPECT_EQ(rtree.end(), iitr);
    EXPECT_EQ(rtree.end(), jitr);
  }
  typedef RadixTreeIterator<IPAddressV6, int> IterType;
  typedef RadixTreeConstIterator<IPAddressV6, int> CIterType;
  EXPECT_EQ(IterType(), rtree.end());
  EXPECT_EQ(CIterType(), crtree.end());
}

// Test composite IPAddress tree
TEST(RadixTree, CombinedV4AndV6) {
  RadixTree<IPAddressV4, int> v4Tree;
  RadixTree<IPAddressV6, int> v6Tree;
  RadixTree<IPAddress, int> ipTree;
  vector<Prefix4> v4Prefixes;
  vector<Prefix6> v6Prefixes;
  setupTestTree4(v4Tree);
  setupTestTree6(v6Tree);
  for (auto& v4entry : v4Tree) {
    ipTree.insert(
        IPAddress::fromBinary(folly::ByteRange(v4entry.ipAddress().bytes(), 4)),
        v4entry.masklen(),
        v4entry.value());
    v4Prefixes.push_back(Prefix4(v4entry.ipAddress(), v4entry.masklen()));
  }
  for (auto& v6entry : v6Tree) {
    ipTree.insert(
        IPAddress::fromBinary(
            folly::ByteRange(v6entry.ipAddress().bytes(), 16)),
        v6entry.masklen(),
        v6entry.value());
    v6Prefixes.push_back(Prefix6(v6entry.ipAddress(), v6entry.masklen()));
  }
  EXPECT_EQ(v4Tree.size() + v6Tree.size(), ipTree.size());
  const auto& ipCtree = ipTree;
  auto ipItr = ipTree.begin();
  auto ipCitr = ipCtree.begin();
  auto v4Itr = v4Tree.begin();
  for (; ipItr != ipTree.end() && v4Itr != v4Tree.end();
       ++ipItr, ++v4Itr, ++ipCitr) {
    v4Itr->equalSansLinks(*(ipItr->node4()));
    // Compare with const iterator
    v4Itr->equalSansLinks(*(ipCitr->node4()));
    // Check if exact match returns the same iterator
    EXPECT_EQ(
        ipItr,
        ipTree.exactMatch(
            IPAddress::fromBinary(
                folly::ByteRange(v4Itr->ipAddress().bytes(), 4)),
            v4Itr->masklen()));
    EXPECT_EQ(
        ipCitr,
        ipCtree.exactMatch(
            IPAddress::fromBinary(
                folly::ByteRange(v4Itr->ipAddress().bytes(), 4)),
            v4Itr->masklen()));
    // Check if longest match returns the same iterator
    EXPECT_EQ(
        ipItr,
        ipTree.longestMatch(
            IPAddress::fromBinary(
                folly::ByteRange(v4Itr->ipAddress().bytes(), 4)),
            v4Itr->masklen()));
    EXPECT_EQ(
        ipCitr,
        ipCtree.longestMatch(
            IPAddress::fromBinary(
                folly::ByteRange(v4Itr->ipAddress().bytes(), 4)),
            v4Itr->masklen()));
    // Compare the trails
    decltype(v4Tree)::VecConstIterators v4Trail;
    decltype(ipTree)::VecConstIterators ipTrail;
    decltype(ipTree)::VecConstIterators ipExactMatchTrail;
    auto v4MatchItr = v4Tree.longestMatchWithTrail(
        v4Itr->ipAddress(),
        v4Itr->masklen(),
        v4Trail,
        true /*include non value nodes*/);
    auto ipMatchItr = ipTree.longestMatchWithTrail(
        ipItr->ipAddress(),
        ipItr->masklen(),
        ipTrail,
        true /*include non value nodes*/);
    auto ipExactMatchItr = ipTree.exactMatchWithTrail(
        ipItr->ipAddress(),
        ipItr->masklen(),
        ipExactMatchTrail,
        true /*include non value nodes*/);
    EXPECT_TRUE(v4MatchItr->equalSansLinks(*ipMatchItr->node4()));
    EXPECT_TRUE(ipMatchItr->node4()->equalSansLinks(*ipExactMatchItr->node4()));
    EXPECT_EQ(trailStr(ipTrail), trailStr(ipExactMatchTrail));
    EXPECT_EQ(trailStr(v4Trail), trailStr(ipTrail));
  }
  EXPECT_TRUE(v4Itr.atEnd());
  EXPECT_FALSE(ipItr.atEnd());
  EXPECT_FALSE(ipCitr.atEnd());
  auto v6Itr = v6Tree.begin();
  for (; v6Itr != v6Tree.end() && ipItr != ipTree.end();
       // Use of post increment deliberate - to invoke that part of the code
       v6Itr++,
       ipItr++,
       ipCitr++) {
    v6Itr->equalSansLinks(*(ipItr->node6()));
    // Compare with const iterator
    v6Itr->equalSansLinks(*(ipCitr->node6()));
    // Check if exact match returns the same iterator
    EXPECT_EQ(
        ipItr,
        ipTree.exactMatch(
            IPAddress::fromBinary(
                folly::ByteRange(v6Itr->ipAddress().bytes(), 16)),
            v6Itr->masklen()));
    EXPECT_EQ(
        ipCitr,
        ipCtree.exactMatch(
            IPAddress::fromBinary(
                folly::ByteRange(v6Itr->ipAddress().bytes(), 16)),
            v6Itr->masklen()));
    // Check if longest match returns the same iterator
    EXPECT_EQ(
        ipItr,
        ipTree.longestMatch(
            IPAddress::fromBinary(
                folly::ByteRange(v6Itr->ipAddress().bytes(), 16)),
            v6Itr->masklen()));
    EXPECT_EQ(
        ipCitr,
        ipCtree.longestMatch(
            IPAddress::fromBinary(
                folly::ByteRange(v6Itr->ipAddress().bytes(), 16)),
            v6Itr->masklen()));

    // Compare the trails
    decltype(v6Tree)::VecConstIterators v6Trail;
    decltype(ipTree)::VecConstIterators ipTrail;
    decltype(ipTree)::VecConstIterators ipExactMatchTrail;
    auto v6MatchItr = v6Tree.longestMatchWithTrail(
        v6Itr->ipAddress(),
        v6Itr->masklen(),
        v6Trail,
        true /*include non value nodes*/);
    auto ipMatchItr = ipTree.longestMatchWithTrail(
        ipItr->ipAddress(),
        ipItr->masklen(),
        ipTrail,
        true /*include non value nodes*/);
    auto ipExactMatchItr = ipTree.exactMatchWithTrail(
        ipItr->ipAddress(),
        ipItr->masklen(),
        ipExactMatchTrail,
        true /*include non value nodes*/);
    EXPECT_TRUE(v6MatchItr->equalSansLinks(*ipMatchItr->node6()));
    EXPECT_TRUE(ipMatchItr->node6()->equalSansLinks(*ipExactMatchItr->node6()));
    EXPECT_EQ(trailStr(ipTrail), trailStr(ipExactMatchTrail));
    EXPECT_EQ(trailStr(v6Trail), trailStr(ipTrail));
  }
  EXPECT_TRUE(v6Itr.atEnd());
  EXPECT_TRUE(ipItr.atEnd());
  EXPECT_TRUE(ipCitr.atEnd());

  // Erase the least specific node in the combined tree and
  // try to look it up after. The iterator returned should
  // point to the end.
  ipTree.erase(IPAddress(ip0_0_0_0), 1);
  {
    typedef RadixTree<IPAddress, int> RadixTree;
    typename RadixTree::ConstIterator exactMatchCitr, longestMatchCitr;
    typename RadixTree::Iterator exactMatchItr, longestMatchItr;
    exactMatchCitr = const_cast<const RadixTree*>(&ipTree)->exactMatch(
        IPAddress(ip0_0_0_0), 1);
    exactMatchItr = ipTree.exactMatch(IPAddress(ip0_0_0_0), 1);
    longestMatchCitr = const_cast<const RadixTree*>(&ipTree)->longestMatch(
        IPAddress(ip0_0_0_0), 1);
    longestMatchItr = ipTree.longestMatch(IPAddress(ip0_0_0_0), 1);
    EXPECT_TRUE(exactMatchItr.atEnd());
    EXPECT_TRUE(exactMatchCitr.atEnd());
    EXPECT_TRUE(longestMatchItr.atEnd());
    EXPECT_TRUE(longestMatchCitr.atEnd());
  }
  // Also delete the above prefix from v4Tree
  v4Tree.erase(ip0_0_0_0, 1);

  // Delete a v4 prefix randomly and compare trees
  auto v4Delete = folly::Random::rand32(v4Prefixes.size() - 1);
  ipTree.erase(IPAddress(v4Prefixes[v4Delete].ip), v4Prefixes[v4Delete].mask);
  v4Tree.erase(v4Prefixes[v4Delete].ip, v4Prefixes[v4Delete].mask);

  // Delete a v6 prefix randomly and compare trees
  auto v6Delete = folly::Random::rand32(v6Prefixes.size() - 1);
  ipTree.erase(IPAddress(v6Prefixes[v6Delete].ip), v6Prefixes[v6Delete].mask);
  v6Tree.erase(v6Prefixes[v6Delete].ip, v6Prefixes[v6Delete].mask);
  // After erase size of the combined tree should be the same as the
  // sum of size of 2 underlying trees
  EXPECT_EQ(v4Tree.size() + v6Tree.size(), ipTree.size());

  ipItr = ipTree.begin();
  v4Itr = v4Tree.begin();
  for (; ipItr != ipTree.end() && v4Itr != v4Tree.end(); ++ipItr, ++v4Itr) {
    v4Itr->equalSansLinks(*(ipItr->node4()));
  }
  EXPECT_TRUE(v4Itr.atEnd());
  EXPECT_FALSE(ipItr.atEnd());

  v6Itr = v6Tree.begin();
  for (; ipItr != ipTree.end() && v6Itr != v6Tree.end(); ++ipItr, ++v6Itr) {
    v6Itr->equalSansLinks(*(ipItr->node6()));
  }
  EXPECT_TRUE(v6Itr.atEnd());
  EXPECT_TRUE(ipItr.atEnd());
}

/*
 * Test comparison of composite tree with only v4 prefixes
 */
TEST(RadixTree, CompositeEmptyV6) {
  RadixTree<IPAddressV4, int> v4Tree;
  RadixTree<IPAddress, int> ipTree;
  vector<Prefix4> v4Prefixes;
  setupTestTree4(v4Tree);
  for (auto& v4entry : v4Tree) {
    ipTree.insert(
        IPAddress::fromBinary(folly::ByteRange(v4entry.ipAddress().bytes(), 4)),
        v4entry.masklen(),
        v4entry.value());
    v4Prefixes.push_back(Prefix4(v4entry.ipAddress(), v4entry.masklen()));
  }
  EXPECT_EQ(v4Prefixes.size(), v4Tree.size());
  EXPECT_EQ(v4Tree.size(), ipTree.size());
  EXPECT_NE(ipTree.begin(), ipTree.end());
  // Since we have only v4 prefixes in the composite tree v4 tree nodes
  // and composite tree nodes should be the same.
  auto v4Itr = v4Tree.begin();
  auto ipItr = ipTree.begin();
  for (; ipItr != ipTree.end() && v4Itr != v4Tree.end(); ++ipItr, ++v4Itr) {
    v4Itr->equalSansLinks(*(ipItr->node4()));
  }
}

/*
 * Test comparison of composite tree with only v6 prefixes
 */
TEST(RadixTree, CompositeEmptyV4) {
  RadixTree<IPAddressV6, int> v6Tree;
  RadixTree<IPAddress, int> ipTree;
  vector<Prefix6> v6Prefixes;
  setupTestTree6(v6Tree);
  for (auto& v6entry : v6Tree) {
    ipTree.insert(
        IPAddress::fromBinary(
            folly::ByteRange(v6entry.ipAddress().bytes(), 16)),
        v6entry.masklen(),
        v6entry.value());
    v6Prefixes.push_back(Prefix6(v6entry.ipAddress(), v6entry.masklen()));
  }
  EXPECT_EQ(v6Prefixes.size(), v6Tree.size());
  EXPECT_EQ(v6Tree.size(), ipTree.size());
  EXPECT_NE(ipTree.begin(), ipTree.end());
  // Since we have only v6 prefixes in the composite tree v6 tree nodes
  // and composite tree nodes should be the same.
  auto v6Itr = v6Tree.begin();
  auto ipItr = ipTree.begin();
  for (; ipItr != ipTree.end() && v6Itr != v6Tree.end(); ++ipItr, ++v6Itr) {
    v6Itr->equalSansLinks(*(ipItr->node6()));
  }
}

/*
 * Test empty radix tree size and iterator
 */
TEST(RadixTree, CompositeEmpty) {
  RadixTree<IPAddress, int> ipTree;
  EXPECT_EQ(0, ipTree.size());
  EXPECT_EQ(ipTree.begin(), ipTree.end());
}

/*
 * Test if we can moveable objects in Radix tree
 */
TEST(RadixTree, MoveConstructible) {
  RadixTree<IPAddressV4, std::unique_ptr<int>> rtree;
  RadixTree<IPAddress, std::unique_ptr<int>> ipRtree;
  std::vector<std::pair<IPAddressV4, uint8_t>> inserted;
  typedef RadixTreeNode<IPAddressV4, std::unique_ptr<int>> IterValueIPv4;
  typedef RadixTreeIterator<IPAddress, std::unique_ptr<int>>::ValueType
      IterValueIP;
  auto counterIPv4 = [](int cnt, const IterValueIPv4& /*i*/) {
    return cnt + 1;
  };
  auto counterIP = [](int cnt, const IterValueIP& /*i*/) { return cnt + 1; };
  auto const kInsertCount = 100;
  set<Prefix4> prefixesSeen;
  // Insert a kInsertCount random prefixes
  for (auto i = 0; i < kInsertCount;) {
    auto mask = folly::Random::rand32(32);
    auto ip = IPAddressV4::fromLongHBO(folly::Random::rand32());
    ip = ip.mask(mask);
    if (prefixesSeen.find(Prefix4(ip, mask)) != prefixesSeen.end()) {
      continue;
    }
    prefixesSeen.insert(Prefix4(ip, mask));
    ++i;
    rtree.insert(ip, mask, std::make_unique<int>(i));
    ipRtree.insert(IPAddress(ip), mask, std::make_unique<int>(i));
    inserted.emplace_back(std::make_pair(ip, mask));
  }
  EXPECT_EQ(kInsertCount, rtree.size());
  EXPECT_EQ(
      kInsertCount, accumulate(rtree.begin(), rtree.end(), 0, counterIPv4));

  // Set value via iterator
  rtree.exactMatch(inserted[0].first, inserted[0].second)
      ->setValue(std::make_unique<int>(42));
  ipRtree.exactMatch(IPAddress(inserted[0].first), inserted[0].second)
      ->setValue(std::make_unique<int>(42));

  auto const kEraseCount = 20;
  // Delete kEraseCount (randomly selected) prefixes
  for (auto i = 0; i < kEraseCount;) {
    auto erase = folly::Random::rand32(inserted.size());
    if (rtree.exactMatch(inserted[erase].first, inserted[erase].second) !=
        rtree.end()) {
      rtree.erase(inserted[erase].first, inserted[erase].second);
      ipRtree.erase(IPAddress(inserted[erase].first), inserted[erase].second);
      ++i;
    }
  }
  EXPECT_EQ(
      kInsertCount - kEraseCount,
      accumulate(rtree.begin(), rtree.end(), 0, counterIPv4));
  EXPECT_EQ(
      kInsertCount - kEraseCount,
      accumulate(ipRtree.begin(), ipRtree.end(), 0, counterIP));
}

TEST(RadixTree, Clone) {
  RadixTree<IPAddressV4, int> v4Tree;
  RadixTree<IPAddressV6, int> v6Tree;
  RadixTree<IPAddress, int> ipTree;

  // Ensure clone() works on empty RadixTrees.
  EXPECT_TRUE(v4Tree == v4Tree.clone());
  EXPECT_TRUE(v6Tree == v6Tree.clone());
  EXPECT_TRUE(ipTree == ipTree.clone());

  vector<Prefix4> v4Prefixes;
  vector<Prefix6> v6Prefixes;
  setupTestTree4(v4Tree);
  setupTestTree6(v6Tree);
  for (auto& v4entry : v4Tree) {
    ipTree.insert(
        IPAddress::fromBinary(folly::ByteRange(v4entry.ipAddress().bytes(), 4)),
        v4entry.masklen(),
        v4entry.value());
    v4Prefixes.push_back(Prefix4(v4entry.ipAddress(), v4entry.masklen()));
  }
  for (auto& v6entry : v6Tree) {
    ipTree.insert(
        IPAddress::fromBinary(
            folly::ByteRange(v6entry.ipAddress().bytes(), 16)),
        v6entry.masklen(),
        v6entry.value());
    v6Prefixes.push_back(Prefix6(v6entry.ipAddress(), v6entry.masklen()));
  }
  auto v4TreeCopy = v4Tree.clone();
  auto v6TreeCopy = v6Tree.clone();
  auto ipTreeCopy = ipTree.clone();
  EXPECT_TRUE(v4Tree == v4TreeCopy);
  EXPECT_TRUE(v6Tree == v6TreeCopy);
  EXPECT_TRUE(ipTree == ipTreeCopy);
}
/*
 * Compare with py-radix
 * Insert a set of random prefixes on both py-radix and our radix tree
 * then compare the two trees. Next from the set of prefixes inserted
 * randomly select some and erase them from both trees. Now compare
 * the trees again. In both cases the trees should be identical.
 */
TEST(RadixTree, PyRadixCompare) {
  struct NoDefaultConstructibleInt {
    explicit NoDefaultConstructibleInt(int val) : val_(val) {}
    NoDefaultConstructibleInt() = delete;
    bool operator==(const NoDefaultConstructibleInt& r) const {
      return val_ == r.val_;
    }
    int val_;
  };
  RadixTree<IPAddressV4, NoDefaultConstructibleInt> rtree;
  PyRadixWrapper<IPAddressV4, NoDefaultConstructibleInt> pyrtree;
  std::vector<std::pair<IPAddressV4, uint8_t>> inserted;
  typedef RadixTreeNode<IPAddressV4, NoDefaultConstructibleInt> IterValue;
  auto counter = [](int cnt, const IterValue& /*i*/) { return cnt + 1; };
  auto const kInsertCount = 1000;
  set<Prefix4> prefixesSeen;
  // Insert a kInsertCount random prefixes
  for (auto i = 0; i < kInsertCount;) {
    auto mask = folly::Random::rand32(32);
    auto ip = IPAddressV4::fromLongHBO(folly::Random::rand32());
    ip = ip.mask(mask);
    if (prefixesSeen.find(Prefix4(ip, mask)) != prefixesSeen.end()) {
      continue;
    }
    prefixesSeen.insert(Prefix4(ip, mask));
    ++i;
    rtree.insert(ip, mask, NoDefaultConstructibleInt(i));
    pyrtree.insert(ip, mask, NoDefaultConstructibleInt(i));
    inserted.emplace_back(std::make_pair(ip, mask));
  }
  EXPECT_EQ(kInsertCount, rtree.size());
  // Compare the trees
  bool ret = (radixTreeEqual<
              IPAddressV4,
              NoDefaultConstructibleInt,
              RadixTreeNode<IPAddressV4, NoDefaultConstructibleInt>,
              radix_node_t,
              RadixTreeNodeAccessor<IPAddressV4, NoDefaultConstructibleInt>,
              PyRadixNodeAccessor<IPAddressV4, NoDefaultConstructibleInt>,
              RadixAndPyRadixNodeEqual<IPAddressV4, NoDefaultConstructibleInt>>(
      rtree.root(), pyrtree.root()));
  EXPECT_TRUE(ret);
  EXPECT_EQ(kInsertCount, accumulate(rtree.begin(), rtree.end(), 0, counter));

  auto const kEraseCount = 200;
  // Delete kEraseCount (randomly selected) prefixes
  for (auto i = 0; i < kEraseCount;) {
    auto erase = folly::Random::rand32(inserted.size());
    if (rtree.exactMatch(inserted[erase].first, inserted[erase].second) !=
        rtree.end()) {
      rtree.erase(inserted[erase].first, inserted[erase].second);
      pyrtree.erase(inserted[erase].first, inserted[erase].second);
      ++i;
    }
  }
  // Compare the trees
  ret = (radixTreeEqual<
         IPAddressV4,
         NoDefaultConstructibleInt,
         RadixTreeNode<IPAddressV4, NoDefaultConstructibleInt>,
         radix_node_t,
         RadixTreeNodeAccessor<IPAddressV4, NoDefaultConstructibleInt>,
         PyRadixNodeAccessor<IPAddressV4, NoDefaultConstructibleInt>,
         RadixAndPyRadixNodeEqual<IPAddressV4, NoDefaultConstructibleInt>>(
      rtree.root(), pyrtree.root()));
  EXPECT_TRUE(ret);
  EXPECT_EQ(
      kInsertCount - kEraseCount,
      accumulate(rtree.begin(), rtree.end(), 0, counter));
}

/*
 * Test iterating over subtrees of RadixTree
 */
TEST(RadixTree, SubTreeIterator) {
  RadixTree<IPAddress, int> rtree;
  vector<pair<string, pair<int, int>>> subnets = {
      // { prefix, {begin, end} } - prefix should contain numbers [begin, end)
      {"1.0.0.0/8", {0, 12}},
      {"1.1.0.0/16", {1, 6}},
      {"1.1.1.0/24", {2, 4}},
      {"1.1.1.254/32", {3, 4}},
      {"1.1.254.0/24", {4, 6}},
      {"1.1.254.1/32", {5, 6}},
      // "1.254.0.0/16", - non-value node
      {"1.254.1.0/24", {6, 10}},
      {"1.254.1.0/28", {7, 10}},
      {"1.254.1.1/32", {8, 9}},
      {"1.254.1.15/32", {9, 10}},
      {"1.254.254.0/24", {10, 12}},
      {"1.254.254.254/32", {11, 12}},
      {"::/0", {12, 19}},
      {"::/1", {13, 16}},
      {"::/2", {14, 15}},
      {"4000::/2", {15, 16}},
      {"8000::/1", {16, 19}},
      {"8000::/2", {17, 18}},
      {"c000::/2", {18, 19}}};
  for (int i = 0; i < subnets.size(); ++i) {
    auto subnet = IPAddress::createNetwork(subnets[i].first);
    int value = subnets[i].second.first;
    EXPECT_TRUE(rtree.insert(subnet.first, subnet.second, value).second);
  }
  /*
   * ipv4 tree:
   *                           1.0.0.0/8 (0)
   *                       /                  \
   *                      /                    \
   *            1.1.0.0/16(1)                  1.254.0.0/16(*)
   *           /           \                   /            \
   *    1.1.1.0/24(2)   1.1.254.0/24(4)       /              \
   *         \             /            1.254.1.0/24(6)    1.254.254.0/24(10)
   *          \       1.1.254.1/32(5)       /                  \
   *       1.1.1.254/32(3)                 /                    \
   *                                 1.254.1.0/28(7)        1.254.254.254/32(11)
   *                                 /        \
   *                       1.254.1.1/32(8)   1.254.1.15(9)
   *
   * ipv6 tree:
   *                        ::/0(12)
   *                    /             \
   *                   /               \
   *             ::/1(13)              8000::/1(16)
   *           /        \              /          \
   *          /          \         8000::/2(17)   c000::/2 (18)
   *      ::/2(14)      4000::/2(15)
   */

  for (int i = 0; i < subnets.size(); ++i) {
    auto subnet = IPAddress::createNetwork(subnets[i].first);
    auto iter = rtree.exactMatch(subnet.first, subnet.second);
    int begin = subnets[i].second.first;
    int end = subnets[i].second.second;
    for (iter = iter.subTreeIterator(); !iter.atEnd(); ++iter, ++begin) {
      EXPECT_EQ(iter->value(), begin);
    }
    EXPECT_EQ(begin, end);
    EXPECT_EQ(iter, rtree.end());
  }
  EXPECT_EQ(rtree.end().subTreeIterator(), rtree.end());
}

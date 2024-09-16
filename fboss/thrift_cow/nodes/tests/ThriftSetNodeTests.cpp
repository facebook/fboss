/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fboss/thrift_cow/visitors/tests/VisitorTestUtils.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include <thrift/lib/cpp2/reflection/testing.h>
#include "fboss/agent/gen-cpp2/switch_config_fatal_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

#include <gtest/gtest.h>
#include <type_traits>

using namespace facebook::fboss;
using namespace facebook::fboss::thrift_cow;

using k = test_tags::strings;
using sk = cfg::switch_config_tags::strings;

TEST(ThriftSetNodeTests, ThriftSetFieldsPrimitivesSimple) {
  ThriftSetFields<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>
      fields;
  ASSERT_EQ(fields.size(), 0);

  ThriftSetFields<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::set<int>>
      fields2;
  ASSERT_EQ(fields2.size(), 0);
}

TEST(ThriftSetNodeTests, ThriftSetFieldsPrimitivesGetSet) {
  ThriftSetFields<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>
      fields;
  fields.emplace(3);
  fields.emplace(7);
  ASSERT_EQ(fields.size(), 2);
  auto it = fields.find(3);
  ASSERT_NE(it, fields.end());
  ASSERT_EQ(*it, 3);

  it = fields.find(7);
  ASSERT_NE(it, fields.end());
  ASSERT_EQ(*it, 7);

  fields.erase(7);
  it = fields.find(7);
  ASSERT_EQ(it, fields.end());
}

TEST(ThriftSetNodeTests, ThriftSetFieldsPrimitivesConstructFromThrift) {
  std::unordered_set<int> data = {1, 2, 5, 99};
  ThriftSetFields<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>
      fields(data);

  ASSERT_EQ(fields.size(), 4);

  ASSERT_NE(fields.find(1), fields.end());
  ASSERT_NE(fields.find(2), fields.end());
  ASSERT_NE(fields.find(5), fields.end());
  ASSERT_NE(fields.find(99), fields.end());
  ASSERT_EQ(fields.find(199), fields.end());

  ASSERT_EQ(fields.toThrift(), data);
}

TEST(ThriftSetNodeTests, ThriftSetNodePrimitivesSimple) {
  ThriftSetNode<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>
      node;
  ASSERT_EQ(node.size(), 0);

  ThriftSetNode<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::set<int>>
      node2;
  ASSERT_EQ(node2.size(), 0);
}

TEST(ThriftSetNodeTests, ThriftSetNodePrimitivesGetSet) {
  ThriftSetNode<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>
      node;
  node.emplace(3);
  node.emplace(7);
  ASSERT_EQ(node.size(), 2);
  auto it = node.find(3);
  ASSERT_NE(it, node.end());
  ASSERT_EQ(*it, 3);

  it = node.find(7);
  ASSERT_NE(it, node.end());
  ASSERT_EQ(*it, 7);

  node.erase(7);
  it = node.find(7);
  ASSERT_EQ(it, node.end());
}

TEST(ThriftSetNodeTests, ThriftSetNodePrimitivesConstructFromThrift) {
  std::unordered_set<int> data = {1, 2, 5, 99};
  ThriftSetNode<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>
      node(data);

  ASSERT_EQ(node.size(), 4);

  ASSERT_NE(node.find(1), node.end());
  ASSERT_NE(node.find(2), node.end());
  ASSERT_NE(node.find(5), node.end());
  ASSERT_NE(node.find(99), node.end());
  ASSERT_EQ(node.find(199), node.end());

  ASSERT_EQ(node.toThrift(), data);
}

TEST(ThriftSetNodeTests, ThriftSetNodePrimitivesVisit) {
  std::unordered_set<int> data = {1, 2, 5, 99};
  ThriftSetNode<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>
      node(data);

  folly::dynamic out;
  auto f = [&out](auto& node) { out = node.toFollyDynamic(); };

  std::vector<std::string> path = {"1"};
  auto result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 1);

  path = {"1", "test"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);

  path = {"2"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 2);
}

TEST(ThriftSetNodeTests, ThriftSetNodePrimitivesVisitMutable) {
  std::unordered_set<int> data = {1, 2, 5, 99};
  ThriftSetNode<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>
      node(data);

  folly::dynamic toWrite, out;
  auto write = [&toWrite](auto& node) { node.fromFollyDynamic(toWrite); };
  auto read = [&out](auto& node) { out = node.toFollyDynamic(); };

  std::vector<std::string> path = {"1"};
  auto result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 1);

  path = {"2"};
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 2);

  toWrite = 3;
  path = {"1"};
  result = visitPath(node, path.begin(), path.end(), write);
  ASSERT_EQ(result, ThriftTraverseResult::VISITOR_EXCEPTION);
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);

  // write to a set member path should be skipped. We need a better
  // solution for this that doesn't rely on checking for constness in
  // the visitor. I will likely create a special
  // ImmutablePrimitiveNode type for set members.
  ASSERT_EQ(out, 1);
}

TEST(ThriftSetNodeTests, ThriftSetNodePrimitivesClone) {
  using TestNodeType = ThriftSetNode<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>;

  std::unordered_set<int> data = {1, 2, 5, 99};

  auto node = std::make_shared<TestNodeType>(data);

  ASSERT_FALSE(node->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());

  auto newNode = node->clone();
  ASSERT_FALSE(newNode->isPublished());
}

TEST(ThriftSetNodeTests, ThriftSetNodePrimitivesModify) {
  using TestNodeType = ThriftSetNode<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>;

  std::unordered_set<int> data = {1, 2, 5, 99};

  auto node = std::make_shared<TestNodeType>(data);

  ASSERT_FALSE(node->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());

  TestNodeType::modify(&node, "1");

  ASSERT_FALSE(node->isPublished());

  // now try modifying a nonexistent key
  ASSERT_EQ(node->size(), 4);
  ASSERT_EQ(node->find(6), node->end());
  TestNodeType::modify(&node, "6");
  ASSERT_EQ(node->size(), 5);
  ASSERT_NE(node->find(6), node->end());
}

TEST(ThriftSetNodeTests, ThriftSetNodePrimitivesRemove) {
  using TestNodeType = ThriftSetNode<
      apache::thrift::type_class::set<apache::thrift::type_class::integral>,
      std::unordered_set<int>>;

  std::unordered_set<int> data = {1, 2, 5, 99};

  auto node = std::make_shared<TestNodeType>(data);

  ASSERT_EQ(node->size(), 4);
  ASSERT_NE(node->find(1), node->end());
  ASSERT_NE(node->find(99), node->end());

  // remove by value
  ASSERT_TRUE(node->remove(1));
  ASSERT_EQ(node->size(), 3);
  ASSERT_EQ(node->find(1), node->end());

  // remove by string
  ASSERT_TRUE(node->remove("99"));
  ASSERT_EQ(node->size(), 2);
  ASSERT_EQ(node->find(99), node->end());

  // verify remove missing value returns false
  ASSERT_FALSE(node->remove("99"));

  // verify remove incompatible value returns false
  ASSERT_FALSE(node->remove("incompatible"));
}

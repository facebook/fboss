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

#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/MapDelta.h"

#include <gtest/gtest.h>
#include <type_traits>

using namespace facebook::fboss;
using namespace facebook::fboss::thrift_cow;

using k = test_tags::strings;
using sk = cfg::switch_config_tags::strings;

namespace {

cfg::L4PortRange buildPortRange(int min, int max) {
  cfg::L4PortRange portRange;
  portRange.min() = min;
  portRange.max() = max;
  return portRange;
}

} // namespace

TEST(ThriftMapNodeTests, ThriftMapFieldsPrimitivesSimple) {
  ThriftMapFields<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>
      fields;
  ASSERT_EQ(fields.size(), 0);
}

TEST(ThriftMapNodeTests, ThriftMapFieldsPrimitivesGetSet) {
  ThriftMapFields<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>
      fields;
  fields.emplace(3, 99);
  ASSERT_EQ(fields.size(), 1);
  ASSERT_EQ(fields.at(3), 99);

  fields.ref(3) = 11;
  ASSERT_EQ(fields.at(3), 11);
}

TEST(ThriftMapNodeTests, ThriftMapFieldsPrimitivesConstructFromThrift) {
  std::unordered_map<int, int> data = {{1, 2}, {5, 99}};
  ThriftMapFields<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>
      fields(data);

  ASSERT_EQ(fields.size(), 2);
  ASSERT_EQ(fields.at(1), 2);
  ASSERT_EQ(fields.at(5), 99);

  ASSERT_EQ(fields.toThrift(), data);
}

TEST(ThriftMapNodeTests, ThriftMapFieldsStructsSimple) {
  ThriftMapFields<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::unordered_map<TestEnum, cfg::L4PortRange>>>
      fields;
  ASSERT_EQ(fields.size(), 0);
}

TEST(ThriftMapNodeTests, ThriftMapFieldsStructsGetSet) {
  ThriftMapFields<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::unordered_map<TestEnum, cfg::L4PortRange>>>
      fields;

  auto portRange1 = buildPortRange(100, 999);
  auto portRange2 = buildPortRange(1000, 9999);

  fields.emplace(TestEnum::FIRST, portRange1);
  ASSERT_EQ(fields.size(), 1);
  ASSERT_EQ(fields.at(TestEnum::FIRST)->toThrift(), portRange1);

  fields.emplace(TestEnum::SECOND, portRange2);
  ASSERT_EQ(fields.size(), 2);
  ASSERT_EQ(fields.at(TestEnum::SECOND)->toThrift(), portRange2);

  fields.ref(TestEnum::FIRST)->template set<sk::min>(500);
  ASSERT_EQ(fields.ref(TestEnum::FIRST)->template get<sk::min>(), 500);
}

TEST(ThriftMapNodeTests, ThriftMapFieldsStructsConstructFromThrift) {
  std::map<TestEnum, cfg::L4PortRange> data = {
      {TestEnum::FIRST, buildPortRange(100, 999)},
      {TestEnum::SECOND, buildPortRange(1000, 9999)}};
  ThriftMapFields<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::map<TestEnum, cfg::L4PortRange>>>
      fields(data);

  ASSERT_EQ(fields.size(), 2);
  ASSERT_EQ(fields.at(TestEnum::FIRST)->toThrift(), data[TestEnum::FIRST]);
  ASSERT_EQ(fields.at(TestEnum::SECOND)->toThrift(), data[TestEnum::SECOND]);

  ASSERT_EQ(fields.toThrift(), data);
}

TEST(ThriftMapNodeTests, ThriftMapNodePrimitivesSimple) {
  ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>
      node;
  ASSERT_EQ(node.size(), 0);
}

TEST(ThriftMapNodeTests, ThriftMapNodePrimitivesGetSet) {
  ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>
      node;
  node.emplace(3, 99);
  ASSERT_EQ(node.size(), 1);
  ASSERT_EQ(node.at(3), 99);

  node.ref(3) = 11;
  ASSERT_EQ(node.at(3), 11);
}

TEST(ThriftMapNodeTests, ThriftMapNodePrimitivesConstructFromThrift) {
  std::unordered_map<int, int> data = {{1, 2}, {5, 99}};
  ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>
      node(data);

  ASSERT_EQ(node.size(), 2);
  ASSERT_EQ(node.at(1), 2);
  ASSERT_EQ(node.at(5), 99);

  ASSERT_EQ(node.toThrift(), data);
}

TEST(ThriftMapNodeTests, ThriftMapNodePrimitivesVisit) {
  std::unordered_map<int, int> data = {{1, 2}, {5, 99}};
  ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>
      node(data);

  folly::dynamic out;
  auto f = [&out](auto& node) { out = node.toFollyDynamic(); };

  std::vector<std::string> path = {"0"};
  auto result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);

  path = {"1"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 2);

  path = {"5"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 99);
}

TEST(ThriftMapNodeTests, ThriftMapNodePrimitivesVisitMutable) {
  std::unordered_map<int, int> data = {{1, 2}, {5, 99}};
  ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>
      node(data);

  folly::dynamic toWrite, out;
  auto write = [&toWrite](auto& node) { node.fromFollyDynamic(toWrite); };
  auto read = [&out](auto& node) { out = node.toFollyDynamic(); };

  std::vector<std::string> path = {"0"};
  auto result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);

  path = {"1"};
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 2);

  path = {"5"};
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 99);

  path = {"1"};
  toWrite = 888;
  result = visitPath(node, path.begin(), path.end(), write);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, toWrite);
}

TEST(ThriftMapNodeTests, ThriftMapNodePrimitivesClone) {
  using TestNodeType = ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>;

  std::unordered_map<int, int> data = {{1, 2}, {5, 99}};

  auto node = std::make_shared<TestNodeType>(data);

  ASSERT_FALSE(node->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());

  auto newNode = node->clone();
  ASSERT_FALSE(newNode->isPublished());
}

TEST(ThriftMapNodeTests, ThriftMapNodePrimitivesModify) {
  using TestNodeType = ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>;

  std::unordered_map<int, int> data = {{1, 2}, {5, 99}};

  auto node = std::make_shared<TestNodeType>(data);

  ASSERT_FALSE(node->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());

  TestNodeType::modify(&node, "1");

  ASSERT_FALSE(node->isPublished());

  // now try modifying a nonexistent key
  ASSERT_EQ(node->size(), 2);
  ASSERT_EQ(node->find(6), node->end());
  TestNodeType::modify(&node, "6");
  ASSERT_EQ(node->size(), 3);
  ASSERT_NE(node->find(6), node->end());
}

TEST(ThriftMapNodeTests, ThriftMapNodePrimitivesRemove) {
  using TestNodeType = ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::integral,
          apache::thrift::type_class::integral>,
      std::unordered_map<int, int>>>;

  std::unordered_map<int, int> data = {{1, 2}, {5, 99}};

  auto node = std::make_shared<TestNodeType>(data);
  ASSERT_EQ(node->size(), 2);
  ASSERT_NE(node->find(1), node->end());
  ASSERT_EQ(node->at(1), 2);

  // remove by key
  ASSERT_TRUE(node->remove(1));
  ASSERT_EQ(node->find(1), node->end());
  ASSERT_EQ(node->size(), 1);

  // reset then remove by string
  node->fromThrift(data);
  ASSERT_NE(node->find(5), node->end());
  ASSERT_EQ(node->at(5), 99);
  ASSERT_TRUE(node->remove("5"));
  ASSERT_EQ(node->find(5), node->end());
  ASSERT_EQ(node->size(), 1);

  // verify removing non-existent keys returns false
  ASSERT_FALSE(node->remove("5"));
  ASSERT_FALSE(node->remove("55"));

  // verify removing incompatible key returns false
  ASSERT_FALSE(node->remove("incompatible"));
}

TEST(ThriftMapNodeTests, ThriftMapNodeStructsSimple) {
  ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::unordered_map<TestEnum, cfg::L4PortRange>>>
      node;
  ASSERT_EQ(node.size(), 0);
}

TEST(ThriftMapNodeTests, ThriftMapNodeStructsGetSet) {
  ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::unordered_map<TestEnum, cfg::L4PortRange>>>
      node;

  auto portRange1 = buildPortRange(100, 999);
  auto portRange2 = buildPortRange(1000, 9999);

  node.emplace(TestEnum::FIRST, portRange1);
  ASSERT_EQ(node.size(), 1);
  ASSERT_EQ(node.at(TestEnum::FIRST)->toThrift(), portRange1);

  node.emplace(TestEnum::SECOND, portRange2);
  ASSERT_EQ(node.size(), 2);
  ASSERT_EQ(node.at(TestEnum::SECOND)->toThrift(), portRange2);

  node.ref(TestEnum::FIRST)->template set<sk::min>(500);
  ASSERT_EQ(node.ref(TestEnum::FIRST)->template get<sk::min>(), 500);
}

TEST(ThriftMapNodeTests, ThriftMapNodeStructsConstructFromThrift) {
  std::map<TestEnum, cfg::L4PortRange> data = {
      {TestEnum::FIRST, buildPortRange(100, 999)},
      {TestEnum::SECOND, buildPortRange(1000, 9999)}};
  ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::map<TestEnum, cfg::L4PortRange>>>
      node(data);

  ASSERT_EQ(node.size(), 2);
  ASSERT_EQ(node.at(TestEnum::FIRST)->toThrift(), data[TestEnum::FIRST]);
  ASSERT_EQ(node.at(TestEnum::SECOND)->toThrift(), data[TestEnum::SECOND]);

  ASSERT_EQ(node.toThrift(), data);
}

TEST(ThriftMapNodeTests, ThriftMapNodeStructsVisit) {
  std::unordered_map<TestEnum, cfg::L4PortRange> data = {
      {TestEnum::FIRST, buildPortRange(100, 999)},
      {TestEnum::SECOND, buildPortRange(1000, 9999)}};
  ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::unordered_map<TestEnum, cfg::L4PortRange>>>
      node(data);

  folly::dynamic out;
  auto f = [&out](auto& node) { out = node.toFollyDynamic(); };

  std::vector<std::string> path = {"FIRST"};
  auto result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  folly::dynamic expected;
  facebook::thrift::to_dynamic(
      expected,
      data[TestEnum::FIRST],
      facebook::thrift::dynamic_format::JSON_1);
  ASSERT_EQ(out, expected);

  // visit by enum value should also work
  path = {"1"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, expected);

  path = {"SECOND"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  facebook::thrift::to_dynamic(
      expected,
      data[TestEnum::SECOND],
      facebook::thrift::dynamic_format::JSON_1);
  ASSERT_EQ(out, expected);

  path = {"2"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, expected);

  path = {"THIRD"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);

  path = {"FIRST", "nonexistent"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::INVALID_STRUCT_MEMBER);

  path = {"1", "nonexistent"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::INVALID_STRUCT_MEMBER);

  path = {"FIRST", "min"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 100);

  path = {"1", "min"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 100);

  path = {"SECOND", "min"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 1000);

  path = {"2", "min"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 1000);
}

TEST(ThriftMapNodeTests, ThriftMapNodeStructsVisitMutable) {
  std::unordered_map<TestEnum, cfg::L4PortRange> data = {
      {TestEnum::FIRST, buildPortRange(100, 999)},
      {TestEnum::SECOND, buildPortRange(1000, 9999)}};
  ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::unordered_map<TestEnum, cfg::L4PortRange>>>
      node(data);

  folly::dynamic toWrite, out;
  auto write = [&toWrite](auto& node) { node.fromFollyDynamic(toWrite); };
  auto read = [&out](auto& node) { out = node.toFollyDynamic(); };

  std::vector<std::string> path = {"FIRST"};
  auto result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  folly::dynamic expected;
  facebook::thrift::to_dynamic(
      expected,
      data[TestEnum::FIRST],
      facebook::thrift::dynamic_format::JSON_1);
  ASSERT_EQ(out, expected);

  path = {"SECOND"};
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  facebook::thrift::to_dynamic(
      expected,
      data[TestEnum::SECOND],
      facebook::thrift::dynamic_format::JSON_1);
  ASSERT_EQ(out, expected);

  path = {"THIRD"};
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);

  path = {"FIRST", "min"};
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 100);

  path = {"SECOND", "min"};
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 1000);

  path = {"FIRST"};
  facebook::thrift::to_dynamic(
      toWrite, buildPortRange(1, 2), facebook::thrift::dynamic_format::JSON_1);
  result = visitPath(node, path.begin(), path.end(), write);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, toWrite);

  path = {"FIRST", "min"};
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 1);

  path = {"SECOND", "min"};
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 1000);
}

TEST(ThriftMapNodeTests, ThriftMapNodeStructsClone) {
  using TestNodeType = ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::unordered_map<TestEnum, cfg::L4PortRange>>>;

  std::unordered_map<TestEnum, cfg::L4PortRange> data = {
      {TestEnum::FIRST, buildPortRange(100, 999)},
      {TestEnum::SECOND, buildPortRange(1000, 9999)}};

  auto node = std::make_shared<TestNodeType>(data);

  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->cref(TestEnum::FIRST)->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());
  ASSERT_TRUE(node->cref(TestEnum::FIRST)->isPublished());

  auto newNode = node->clone();
  ASSERT_FALSE(newNode->isPublished());
  ASSERT_TRUE(node->cref(TestEnum::FIRST)->isPublished());
}

TEST(ThriftMapNodeTests, ThriftMapNodeStructsModify) {
  using TestNodeType = ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::unordered_map<TestEnum, cfg::L4PortRange>>>;

  std::unordered_map<TestEnum, cfg::L4PortRange> data = {
      {TestEnum::FIRST, buildPortRange(100, 999)},
      {TestEnum::SECOND, buildPortRange(1000, 9999)}};

  auto node = std::make_shared<TestNodeType>(data);

  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->cref(TestEnum::FIRST)->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());
  ASSERT_TRUE(node->cref(TestEnum::FIRST)->isPublished());

  TestNodeType::modify(&node, "1");

  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->cref(TestEnum::FIRST)->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());
  ASSERT_TRUE(node->cref(TestEnum::FIRST)->isPublished());
  ASSERT_TRUE(node->cref(TestEnum::SECOND)->isPublished());

  // also test using enum name
  TestNodeType::modify(&node, "SECOND");

  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->cref(TestEnum::SECOND)->isPublished());

  // now try modifying a nonexistent key
  ASSERT_EQ(node->size(), 2);
  ASSERT_EQ(node->find(TestEnum::THIRD), node->end());
  TestNodeType::modify(&node, "THIRD");
  ASSERT_EQ(node->size(), 3);
  ASSERT_NE(node->find(TestEnum::THIRD), node->end());
}

TEST(ThriftMapNodeTests, MapDelta) {
  using Map = ThriftMapNode<ThriftMapTraits<
      apache::thrift::type_class::map<
          apache::thrift::type_class::enumeration,
          apache::thrift::type_class::structure>,
      std::unordered_map<TestEnum, cfg::L4PortRange>>>;

  std::unordered_map<TestEnum, cfg::L4PortRange> data = {
      {TestEnum::FIRST, buildPortRange(1000, 1999)},
      {TestEnum::SECOND, buildPortRange(2000, 2999)}};

  auto map = std::make_shared<Map>(data);
  auto map1 = map->clone();
  map1->remove(TestEnum::SECOND);

  auto map2 = map->clone();
  map2->emplace(TestEnum::THIRD, buildPortRange(3000, 3999));

  auto map3 = map->clone();
  map3->remove(TestEnum::FIRST);
  map3->emplace(TestEnum::FIRST, buildPortRange(1001, 1999));

  auto addedDelta = ThriftMapDelta(map.get(), map2.get());
  auto removedDelta = ThriftMapDelta(map.get(), map1.get());
  auto changedDelta = ThriftMapDelta(map.get(), map3.get());

  DeltaFunctions::forEachAdded(addedDelta, [&](auto addedNode) {
    auto thrift = addedNode->toThrift();
    EXPECT_EQ(*thrift.min(), 3000);
  });

  DeltaFunctions::forEachRemoved(removedDelta, [&](auto removedNode) {
    EXPECT_EQ(removedNode->toThrift(), buildPortRange(2000, 2999));
  });
  DeltaFunctions::forEachChanged(changedDelta, [&](auto oldNode, auto newNode) {
    EXPECT_EQ(oldNode->toThrift(), buildPortRange(1000, 1999));
    EXPECT_EQ(newNode->toThrift(), buildPortRange(1001, 1999));
  });
}

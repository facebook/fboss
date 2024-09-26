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

namespace {

cfg::L4PortRange buildPortRange(int min, int max) {
  cfg::L4PortRange portRange;
  portRange.min() = min;
  portRange.max() = max;
  return portRange;
}

} // namespace

TEST(ThriftStructNodeTests, ThriftStructFieldsSimple) {
  ThriftStructFields<TestStruct> fields;
  ASSERT_EQ(fields.get<k::inlineBool>(), false);
}

TEST(ThriftStructNodeTests, ThriftStructFieldsGetSet) {
  ThriftStructFields<TestStruct> fields;
  ASSERT_EQ(fields.get<k::inlineBool>(), false);
  ASSERT_EQ(fields.get<k::inlineInt>(), 0);
  ASSERT_EQ(fields.get<k::inlineString>(), "");

  fields.set<k::inlineBool>(true);
  fields.set<k::inlineInt>(666);
  fields.set<k::inlineString>("HelloTest");

  ASSERT_EQ(fields.get<k::inlineBool>(), true);
  ASSERT_EQ(fields.get<k::inlineInt>(), 666);
  ASSERT_EQ(fields.get<k::inlineString>(), "HelloTest");
}

TEST(ThriftStructNodeTests, ThriftStructFieldsConstructFromThrift) {
  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";

  ThriftStructFields<TestStruct> fields(data);

  ASSERT_EQ(fields.get<k::inlineBool>(), true);
  ASSERT_EQ(fields.get<k::inlineInt>(), 123);
  ASSERT_EQ(fields.get<k::inlineString>(), "HelloThere");
  ASSERT_FALSE(fields.get<k::optionalInt>());
  ASSERT_FALSE(fields.get<k::optionalStruct>());

  ASSERT_EQ(fields.toThrift(), data);
}

TEST(ThriftStructNodeTests, ThriftStructFieldsOptionalPrimitive) {
  ThriftStructFields<TestStruct> fields;

  auto optionalInt = fields.get<k::optionalInt>();
  ASSERT_FALSE(optionalInt.has_value());

  fields.set<k::optionalInt>(99);
  optionalInt = fields.get<k::optionalInt>();
  ASSERT_TRUE(optionalInt.has_value());
  ASSERT_EQ(*optionalInt, 99);

  // verify fromThrift with the field unset takes
  auto val = fields.toThrift();
  val.optionalInt().reset();
  fields.fromThrift(std::move(val));
  optionalInt = fields.get<k::optionalInt>();
  ASSERT_FALSE(optionalInt.has_value());
}

TEST(ThriftStructNodeTests, ThriftStructFieldsOptionalStruct) {
  ThriftStructFields<TestStruct> fields;

  // checks optional child struct defaults to nullptr shared_ptr
  auto optionalStruct = fields.get<k::optionalStruct>();
  ASSERT_FALSE(optionalStruct);

  // now create an optional child
  auto portRange = buildPortRange(100, 999);

  fields.constructMember<k::optionalStruct>(portRange);
  optionalStruct = fields.get<k::optionalStruct>();
  ASSERT_TRUE(optionalStruct);
  ASSERT_EQ(optionalStruct->toThrift(), portRange);

  // verify fromThrift with the field unset takes
  auto val = fields.toThrift();
  val.optionalStruct().reset();
  fields.fromThrift(std::move(val));
  optionalStruct = fields.get<k::optionalStruct>();
  ASSERT_FALSE(optionalStruct);
}

TEST(ThriftStructNodeTests, ThriftStructFieldsRemove) {
  auto portRange = buildPortRange(100, 999);

  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";
  data.inlineStruct() = std::move(portRange);
  data.optionalInt() = 456;
  data.optionalStruct() = buildPortRange(1, 2);

  ThriftStructFields<TestStruct> fields(data);

  // test remove by name
  ASSERT_TRUE(fields.template ref<k::optionalInt>());
  ASSERT_TRUE(fields.template remove<k::optionalInt>());
  ASSERT_FALSE(fields.template ref<k::optionalInt>());

  // test remove by token
  ASSERT_TRUE(fields.template ref<k::optionalStruct>());
  ASSERT_TRUE(fields.remove("optionalStruct"));
  ASSERT_FALSE(fields.template ref<k::optionalStruct>());

  // remove already missing field returns false
  ASSERT_FALSE(fields.remove("optionalStruct"));

  // test remove by non-existent token
  ASSERT_THROW(fields.remove("nonexistentField"), std::runtime_error);

  // test remove non-optional field
  ASSERT_THROW(fields.remove("inlineBool"), std::runtime_error);
}

TEST(ThriftStructNodeTests, ThriftStructNodeSimple) {
  ThriftStructNode<TestStruct> node;
  ASSERT_EQ(node.get<k::inlineBool>(), false);
}

TEST(ThriftStructNodeTests, ThriftStructNodeGetSet) {
  ThriftStructNode<TestStruct> node;
  ASSERT_EQ(node.get<k::inlineBool>(), false);
  ASSERT_EQ(node.get<k::inlineInt>(), 0);
  ASSERT_EQ(node.get<k::inlineString>(), "");

  node.set<k::inlineBool>(true);
  node.set<k::inlineInt>(666);
  node.set<k::inlineString>("HelloTest");

  ASSERT_EQ(node.get<k::inlineBool>(), true);
  ASSERT_EQ(node.get<k::inlineInt>(), 666);
  ASSERT_EQ(node.get<k::inlineString>(), "HelloTest");
}

TEST(ThriftStructNodeTests, ThriftStructNodeSetChild) {
  auto portRange = buildPortRange(100, 999);

  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";
  data.inlineStruct() = std::move(portRange);

  ThriftStructNode<TestStruct> node(data);

  node.set<k::inlineBool>(true);
  node.set<k::inlineInt>(666);
  node.set<k::inlineString>("HelloTest");

  ASSERT_EQ(node.get<k::inlineBool>(), true);
  ASSERT_EQ(node.get<k::inlineInt>(), 666);
  ASSERT_EQ(node.get<k::inlineString>(), "HelloTest");
  ASSERT_EQ(node.get<k::inlineStruct>()->toThrift(), *data.inlineStruct());

  cfg::L4PortRange portRange2;
  portRange2.min() = 1000;
  portRange2.max() = 9999;

  // test we can set full struct children
  node.template set<k::inlineStruct>(portRange2);
  ASSERT_EQ(node.get<k::inlineStruct>()->toThrift(), portRange2);
}

TEST(ThriftStructNodeTests, ThriftStructNodeSetAlreadyPublished) {
  ThriftStructNode<TestStruct> node;
  ASSERT_EQ(node.get<k::inlineBool>(), false);
  ASSERT_EQ(node.get<k::inlineInt>(), 0);
  ASSERT_EQ(node.get<k::inlineString>(), "");

  node.publish();

  // fail to set a field on a published node
  ASSERT_DEATH(node.set<k::inlineBool>(true), "");

  // fail to set a field on a published child node
  auto child = node.get<k::inlineStruct>();
  ASSERT_DEATH(child->set<sk::invert>(true), "");
}

TEST(ThriftStructNodeTests, ThriftStructNodeVisit) {
  auto portRange = buildPortRange(100, 999);

  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";
  data.inlineStruct() = std::move(portRange);

  ThriftStructNode<TestStruct> node(data);

  folly::dynamic out;
  auto f = [&out](auto& node) { out = node.toFollyDynamic(); };

  std::vector<std::string> path = {"inlineBool"};
  auto result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, true);

  path = {"inlineStruct", "min"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 100);

  path = {"inlineStruct", "max"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, 999);

  path = {"inlineStruct"};
  result = visitPath(node, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, node.template get<k::inlineStruct>()->toFollyDynamic());
}

TEST(ThriftStructNodeTests, ThriftStructNodeVisitMutable) {
  auto portRange = buildPortRange(100, 999);

  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";
  data.inlineStruct() = std::move(portRange);

  ThriftStructNode<TestStruct> node(data);

  folly::dynamic toWrite, out;
  auto write = [&toWrite](auto& node) { node.fromFollyDynamic(toWrite); };
  auto read = [&out](auto& node) { out = node.toFollyDynamic(); };

  std::vector<std::string> path = {"inlineBool"};
  toWrite = false;
  auto result = visitPath(node, path.begin(), path.end(), write);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(out, false);

  path = {"inlineStruct", "min"};
  toWrite = 855;
  result = visitPath(node, path.begin(), path.end(), write);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(out, 855);

  path = {"inlineStruct", "max"};
  toWrite = 1055;
  result = visitPath(node, path.begin(), path.end(), write);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  result = visitPath(node, path.begin(), path.end(), read);
  ASSERT_EQ(out, 1055);
}

TEST(ThriftStructNodeTests, ThriftStructNodeClone) {
  auto portRange = buildPortRange(100, 999);

  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";
  data.inlineStruct() = std::move(portRange);

  auto node = std::make_shared<ThriftStructNode<TestStruct>>(data);

  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->template cref<k::inlineStruct>()->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());
  ASSERT_TRUE(node->template cref<k::inlineStruct>()->isPublished());

  auto newNode = node->clone();
  ASSERT_FALSE(newNode->isPublished());
  ASSERT_TRUE(newNode->template cref<k::inlineStruct>()->isPublished());
}

TEST(ThriftStructNodeTests, ThriftStructNodeModify) {
  auto portRange = buildPortRange(100, 999);

  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";
  data.inlineStruct() = std::move(portRange);

  auto node = std::make_shared<ThriftStructNode<TestStruct>>(data);

  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->template cref<k::inlineStruct>()->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());
  ASSERT_TRUE(node->template cref<k::inlineStruct>()->isPublished());

  ThriftStructNode<TestStruct>::modify(&node, "inlineStruct");
  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->template cref<k::inlineStruct>()->isPublished());

  // now try modifying a missing optional field
  ASSERT_FALSE(node->template isSet<k::optionalString>());
  ThriftStructNode<TestStruct>::modify(&node, "optionalString");
  ASSERT_TRUE(node->template isSet<k::optionalString>());

  node = std::make_shared<ThriftStructNode<TestStruct>>(data);
  node->publish();
  auto& inLineBool = ThriftStructNode<TestStruct>::modify<k::inlineBool>(&node);
  EXPECT_FALSE(node->isPublished());
  inLineBool->set(false);

  auto& optionalStruct1 =
      ThriftStructNode<TestStruct>::modify<k::optionalStruct>(&node);
  EXPECT_FALSE(optionalStruct1->isPublished());
  optionalStruct1->fromThrift(buildPortRange(1, 10));

  node->publish();
  EXPECT_TRUE(node->isPublished());

  auto obj1 = node->toThrift();
  EXPECT_FALSE(*obj1.inlineBool());
  EXPECT_TRUE(obj1.optionalStruct().has_value());
  EXPECT_EQ(obj1.optionalStruct().value(), buildPortRange(1, 10));

  auto& optionalStruct2 = node->modify<k::optionalStruct>(&node);
  optionalStruct2.reset();
  node->publish();

  auto obj2 = node->toThrift();
  EXPECT_FALSE(obj2.optionalStruct().has_value());
}

TEST(ThriftStructNodeTests, ThriftStructNodeRemove) {
  auto portRange = buildPortRange(100, 999);

  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";
  data.inlineStruct() = std::move(portRange);
  data.optionalInt() = 456;
  data.optionalStruct() = buildPortRange(1, 2);

  auto node = std::make_shared<ThriftStructNode<TestStruct>>(data);

  // test remove by name
  ASSERT_TRUE(node->template ref<k::optionalInt>());
  ASSERT_TRUE(node->template remove<k::optionalInt>());
  ASSERT_FALSE(node->template ref<k::optionalInt>());

  // test remove by token
  ASSERT_TRUE(node->template ref<k::optionalStruct>());
  ASSERT_TRUE(node->remove("optionalStruct"));
  ASSERT_FALSE(node->template ref<k::optionalStruct>());

  // remove already missing field returns false
  ASSERT_FALSE(node->remove("optionalStruct"));

  // test remove by non-existent token
  ASSERT_THROW(node->remove("nonexistentField"), std::runtime_error);

  // test remove non-optional field
  ASSERT_THROW(node->remove("inlineBool"), std::runtime_error);
}

TEST(ThriftStructNodeTests, ThriftStructFieldsEncode) {
  auto portRange = buildPortRange(100, 999);
  TestUnion unionData;
  unionData.inlineString_ref() = "UnionData";

  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";
  data.inlineStruct() = portRange;
  data.inlineVariant() = std::move(unionData);

  ThriftStructFields<TestStruct> fields(data);

  ASSERT_EQ(fields.toThrift(), data);

  auto encoded = fields.encode(fsdb::OperProtocol::COMPACT);
  auto decoded = apache::thrift::CompactSerializer::deserialize<TestStruct>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);

  encoded = fields.encode(fsdb::OperProtocol::SIMPLE_JSON);
  decoded = apache::thrift::SimpleJSONSerializer::deserialize<TestStruct>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);

  encoded = fields.encode(fsdb::OperProtocol::BINARY);
  decoded = apache::thrift::BinarySerializer::deserialize<TestStruct>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);
}

TEST(ThriftStructNodeTests, ThriftStructNodeEncode) {
  auto portRange = buildPortRange(100, 999);

  TestUnion unionData;
  unionData.inlineString_ref() = "UnionData";

  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";
  data.inlineStruct() = std::move(portRange);
  data.inlineVariant() = std::move(unionData);

  auto node = std::make_shared<ThriftStructNode<TestStruct>>(data);
  ASSERT_EQ(node->toThrift(), data);

  auto encoded = node->encode(fsdb::OperProtocol::COMPACT);
  auto decoded = apache::thrift::CompactSerializer::deserialize<TestStruct>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);

  encoded = node->encode(fsdb::OperProtocol::SIMPLE_JSON);
  decoded = apache::thrift::SimpleJSONSerializer::deserialize<TestStruct>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);

  encoded = node->encode(fsdb::OperProtocol::BINARY);
  decoded = apache::thrift::BinarySerializer::deserialize<TestStruct>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);
}

TEST(ThriftStructNodeTests, UnsignedInteger) {
  ThriftStructFields<TestStruct> fields;
  using UnderlyingType = folly::remove_cvref_t<
      decltype(*fields.get<k::unsigned_int64>())>::ThriftType;
  static_assert(std::is_same_v<UnderlyingType, uint64_t>);
}

TEST(ThriftStructNodeTests, ReferenceWrapper) {
  auto portRange = buildPortRange(100, 999);

  TestUnion unionData;
  unionData.inlineString_ref() = "UnionData";

  TestStruct data;
  data.inlineBool() = true;
  data.inlineInt() = 123;
  data.inlineString() = "HelloThere";
  data.inlineStruct() = std::move(portRange);
  data.inlineVariant() = std::move(unionData);

  auto node = std::make_shared<ThriftStructNode<TestStruct>>(data);

  using NodeType = decltype(node);
  auto ref = detail::ReferenceWrapper<NodeType>(node);

  ref->set<k::inlineInt>(124);
  (*ref).set<k::inlineBool>(false);

  EXPECT_EQ(node->get<k::inlineInt>(), 124);
  EXPECT_EQ(node->get<k::inlineBool>(), false);

  auto& node0 = *ref;

  EXPECT_EQ(&node0, node.get());

  ref.reset();
  EXPECT_EQ(node, nullptr);

  EXPECT_FALSE(ref);
}

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
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include <thrift/lib/cpp2/reflection/testing.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/nodes/Types.h"

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

TEST(ThriftUnionNodeTests, ThriftUnionFieldsSimple) {
  ThriftUnionFields<TestUnion> fields;
  ASSERT_EQ(fields.type(), TestUnion::Type::__EMPTY__);
  ASSERT_THROW(fields.get<k::inlineBool>(), std::bad_variant_access);
}

TEST(ThriftUnionNodeTests, ThriftUnionFieldsGetSet) {
  ThriftUnionFields<TestUnion> fields;
  ASSERT_EQ(fields.type(), TestUnion::Type::__EMPTY__);

  fields.set<k::inlineBool>(true);
  ASSERT_EQ(fields.type(), TestUnion::Type::inlineBool);
  ASSERT_EQ(fields.get<k::inlineBool>(), true);

  fields.set<k::inlineInt>(666);
  ASSERT_EQ(fields.type(), TestUnion::Type::inlineInt);
  ASSERT_EQ(fields.get<k::inlineInt>(), 666);
  ASSERT_THROW(fields.get<k::inlineBool>(), std::bad_variant_access);

  fields.set<k::inlineString>("HelloTest");
  ASSERT_EQ(fields.type(), TestUnion::Type::inlineString);
  ASSERT_EQ(fields.get<k::inlineString>(), "HelloTest");
  ASSERT_THROW(fields.get<k::inlineBool>(), std::bad_variant_access);
  ASSERT_THROW(fields.get<k::inlineInt>(), std::bad_variant_access);
}

TEST(ThriftUnionNodeTests, ThriftUnionFieldsRemove) {
  ThriftUnionFields<TestUnion> fields;
  ASSERT_EQ(fields.type(), TestUnion::Type::__EMPTY__);

  fields.template set<k::inlineBool>(true);
  ASSERT_EQ(fields.type(), TestUnion::Type::inlineBool);
  ASSERT_EQ(fields.template get<k::inlineBool>(), true);

  // remove by type
  ASSERT_TRUE(fields.template remove<k::inlineBool>());
  ASSERT_EQ(fields.type(), TestUnion::Type::__EMPTY__);

  // add back, then remove by string
  fields.template set<k::inlineBool>(true);
  ASSERT_EQ(fields.type(), TestUnion::Type::inlineBool);
  ASSERT_EQ(fields.template get<k::inlineBool>(), true);
  ASSERT_TRUE(fields.remove("inlineBool"));
  ASSERT_EQ(fields.type(), TestUnion::Type::__EMPTY__);

  // verify we return false if we try to remove a field that isn't set
  ASSERT_FALSE(fields.remove("inlineBool"));

  // even if another field is set
  fields.template set<k::inlineInt>(99);
  ASSERT_FALSE(fields.remove("inlineBool"));

  // error on removing non-existent field
  ASSERT_THROW(fields.remove("nonexistent"), std::runtime_error);
}

TEST(ThriftUnionNodeTests, ThriftUnionFieldsConstructFromThrift) {
  TestUnion data;
  data.inlineString_ref() = "HelloThere";

  ThriftUnionFields<TestUnion> fields(data);

  ASSERT_THROW(fields.get<k::inlineBool>(), std::bad_variant_access);
  ASSERT_THROW(fields.get<k::inlineInt>(), std::bad_variant_access);
  ASSERT_EQ(fields.get<k::inlineString>(), "HelloThere");

  ASSERT_EQ(fields.toThrift(), data);
}

TEST(ThriftUnionNodeTests, ThriftUnionNodeSimple) {
  ThriftUnionNode<TestUnion> fields;
  ASSERT_EQ(fields.type(), TestUnion::Type::__EMPTY__);
  ASSERT_THROW(fields.get<k::inlineBool>(), std::bad_variant_access);
}

TEST(ThriftUnionNodeTests, ThriftUnionNodeGetSet) {
  ThriftUnionNode<TestUnion> fields;
  ASSERT_EQ(fields.type(), TestUnion::Type::__EMPTY__);

  fields.set<k::inlineBool>(true);
  ASSERT_EQ(fields.type(), TestUnion::Type::inlineBool);
  ASSERT_EQ(fields.get<k::inlineBool>(), true);

  fields.set<k::inlineInt>(666);
  ASSERT_EQ(fields.type(), TestUnion::Type::inlineInt);
  ASSERT_EQ(fields.get<k::inlineInt>(), 666);
  ASSERT_THROW(fields.get<k::inlineBool>(), std::bad_variant_access);

  fields.set<k::inlineString>("HelloTest");
  ASSERT_EQ(fields.type(), TestUnion::Type::inlineString);
  ASSERT_EQ(fields.get<k::inlineString>(), "HelloTest");
  ASSERT_THROW(fields.get<k::inlineBool>(), std::bad_variant_access);
  ASSERT_THROW(fields.get<k::inlineInt>(), std::bad_variant_access);
}

TEST(ThriftUnionNodeTests, ThriftUnionNodeConstructFromThrift) {
  TestUnion data;
  data.inlineString_ref() = "HelloThere";

  ThriftUnionNode<TestUnion> fields(data);

  ASSERT_THROW(fields.get<k::inlineBool>(), std::bad_variant_access);
  ASSERT_THROW(fields.get<k::inlineInt>(), std::bad_variant_access);
  ASSERT_EQ(fields.get<k::inlineString>(), "HelloThere");

  ASSERT_EQ(fields.toThrift(), data);
}

TEST(ThriftUnionNodeTests, ThriftUnionNodeVisit) {
  TestUnion data;
  data.inlineString_ref() = "HelloThere";

  ThriftUnionNode<TestUnion> fields(data);

  folly::dynamic out;
  auto f = [&out](auto& node, auto /*begin*/, auto /*end*/) {
    out = node.toFollyDynamic();
  };

  std::vector<std::string> path = {"inlineBool"};
  auto result = visitPath(fields, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::INCORRECT_VARIANT_MEMBER);

  path = {"inlineInt"};
  result = visitPath(fields, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::INCORRECT_VARIANT_MEMBER);

  path = {"inlineString"};
  result = visitPath(fields, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::OK);
  ASSERT_EQ(out, "HelloThere");

  path = {"inlineStruct", "min"};
  result = visitPath(fields, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::INCORRECT_VARIANT_MEMBER);

  path = {"inlineStruct", "max"};
  result = visitPath(fields, path.begin(), path.end(), f);
  ASSERT_EQ(result, ThriftTraverseResult::INCORRECT_VARIANT_MEMBER);
}

TEST(ThriftUnionNodeTests, ThriftUnionNodeClone) {
  auto portRange = buildPortRange(100, 999);

  TestUnion data;
  data.inlineStruct_ref() = portRange;

  auto node = std::make_shared<ThriftUnionNode<TestUnion>>(data);

  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->template cref<k::inlineStruct>()->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());
  ASSERT_TRUE(node->template cref<k::inlineStruct>()->isPublished());

  auto newNode = node->clone();
  ASSERT_FALSE(newNode->isPublished());
  ASSERT_TRUE(newNode->template cref<k::inlineStruct>()->isPublished());
}

TEST(ThriftUnionNodeTests, ThriftUnionNodeModify) {
  auto portRange = buildPortRange(100, 999);

  TestUnion data;
  data.inlineStruct_ref() = portRange;

  auto node = std::make_shared<ThriftUnionNode<TestUnion>>(data);

  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->template cref<k::inlineStruct>()->isPublished());

  node->publish();

  ASSERT_TRUE(node->isPublished());
  ASSERT_TRUE(node->template cref<k::inlineStruct>()->isPublished());

  ThriftUnionNode<TestUnion>::modify(&node, "inlineStruct");

  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->template cref<k::inlineStruct>()->isPublished());

  // now change the field
  ThriftUnionNode<TestUnion>::modify(&node, "inlineInt");
  ASSERT_FALSE(node->isPublished());
  ASSERT_FALSE(node->template isSet<k::inlineStruct>());
  ASSERT_TRUE(node->template isSet<k::inlineInt>());

  node->publish();

  // try changing if already published
  ThriftUnionNode<TestUnion>::modify(&node, "inlineStruct");
  ASSERT_FALSE(node->isPublished());
  ASSERT_TRUE(node->template isSet<k::inlineStruct>());
  ASSERT_FALSE(node->template isSet<k::inlineInt>());
  ASSERT_FALSE(node->template cref<k::inlineStruct>()->isPublished());
}

TEST(ThriftUnionNodeTests, ThriftUnionNodeRemove) {
  TestUnion data;
  data.inlineBool_ref() = true;

  auto node = std::make_shared<ThriftUnionNode<TestUnion>>(data);

  ASSERT_EQ(node->type(), TestUnion::Type::inlineBool);
  ASSERT_EQ(node->template get<k::inlineBool>(), true);

  // remove by type
  ASSERT_TRUE(node->template remove<k::inlineBool>());
  ASSERT_EQ(node->type(), TestUnion::Type::__EMPTY__);

  // add back, then remove by string
  node->template set<k::inlineBool>(true);
  ASSERT_EQ(node->type(), TestUnion::Type::inlineBool);
  ASSERT_EQ(node->template get<k::inlineBool>(), true);
  ASSERT_TRUE(node->remove("inlineBool"));
  ASSERT_EQ(node->type(), TestUnion::Type::__EMPTY__);

  // verify we return false if we try to remove a field that isn't set
  ASSERT_FALSE(node->remove("inlineBool"));

  // even if another field is set
  node->template set<k::inlineInt>(99);
  ASSERT_FALSE(node->remove("inlineBool"));

  // error on removing non-existent field
  ASSERT_THROW(node->remove("nonexistent"), std::runtime_error);
}

TEST(ThriftUnionNodeTests, ThriftUnionFieldsEncode) {
  TestUnion data;
  data.inlineString_ref() = "UnionData";

  ThriftUnionFields<TestUnion> fields(data);

  ASSERT_EQ(fields.toThrift(), data);

  auto encoded = fields.encode(fsdb::OperProtocol::COMPACT);
  auto decoded = apache::thrift::CompactSerializer::deserialize<TestUnion>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);

  encoded = fields.encode(fsdb::OperProtocol::SIMPLE_JSON);
  decoded = apache::thrift::SimpleJSONSerializer::deserialize<TestUnion>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);

  encoded = fields.encode(fsdb::OperProtocol::BINARY);
  decoded = apache::thrift::BinarySerializer::deserialize<TestUnion>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);
}

TEST(ThriftUnionNodeTests, ThriftUnionNodeEncode) {
  TestUnion data;
  data.inlineString_ref() = "UnionData";

  auto node = std::make_shared<ThriftUnionNode<TestUnion>>(data);
  ASSERT_EQ(node->toThrift(), data);

  auto encoded = node->encode(fsdb::OperProtocol::COMPACT);
  auto decoded = apache::thrift::CompactSerializer::deserialize<TestUnion>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);

  encoded = node->encode(fsdb::OperProtocol::SIMPLE_JSON);
  decoded = apache::thrift::SimpleJSONSerializer::deserialize<TestUnion>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);

  encoded = node->encode(fsdb::OperProtocol::BINARY);
  decoded = apache::thrift::BinarySerializer::deserialize<TestUnion>(
      encoded.toStdString());
  ASSERT_EQ(decoded, data);
}

// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/visitors/PathVisitor.h>
#include <thrift/lib/cpp2/reflection/folly_dynamic.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

using folly::dynamic;
using namespace facebook::fboss;
using namespace facebook::fboss::thrift_cow;
using k = facebook::fboss::test_tags::strings;

namespace {

TestStruct createTestStruct() {
  dynamic testDyn = dynamic::object("inlineBool", true)("inlineInt", 54)(
      "inlineString",
      "testname")("optionalString", "bla")("inlineStruct", dynamic::object("min", 10)("max", 20))("inlineVariant", dynamic::object("inlineInt", 99))("mapOfEnumToStruct", dynamic::object("3", dynamic::object("min", 100)("max", 200)));

  return apache::thrift::from_dynamic<TestStruct>(
      testDyn, apache::thrift::dynamic_format::JSON_1);
}

} // namespace

TEST(PathVisitorTests, AccessField) {
  auto structA = createTestStruct();

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  folly::dynamic dyn;
  auto processPath = [&dyn](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    dyn = node.toFollyDynamic();
  };
  std::vector<std::string> path{"inlineInt"};
  auto result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(dyn.asInt(), 54);

  path = {"2"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(dyn.asInt(), 54);
}

TEST(PathVisitorTests, AccessFieldInContainer) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  cfg::L4PortRange got;
  auto processPath = [&got](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    got = apache::thrift::from_dynamic<cfg::L4PortRange>(
        node.toFollyDynamic(), apache::thrift::dynamic_format::JSON_1);
  };

  std::vector<std::string> path{"mapOfEnumToStruct", "3"};
  auto result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);

  path = {"15", "3"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);
}

TEST(PathVisitorTests, TraversalModeFull) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  std::set<std::string> got;
  auto processPath = [&got](auto& node, auto begin, auto end) {
    got.insert("/" + folly::join('/', std::vector<std::string>(begin, end)));
  };
  std::vector<std::string> path{"mapOfEnumToStruct", "3"};
  auto result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::FULL, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_THAT(
      got,
      ::testing::ContainerEq(
          std::set<std::string>{"/", "/3", "/mapOfEnumToStruct/3"}));
}

TEST(PathVisitorTests, AccessOptional) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  std::string got;
  auto processPath = [&got](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    got = node.toFollyDynamic().asString();
  };

  std::vector<std::string> path{"optionalString"};
  auto result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(got, "bla");
  structA.optionalString().reset();
  nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  got.clear();
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);
  EXPECT_TRUE(got.empty());
}

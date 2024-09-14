// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/visitors/DeltaVisitor.h>
#include <fboss/thrift_cow/visitors/tests/VisitorTestUtils.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

using folly::dynamic;

namespace {
using namespace facebook::fboss;

using PathTagSet = std::set<std::pair<std::string, thrift_cow::DeltaElemTag>>;
} // namespace

namespace facebook::fboss::thrift_cow::test {

TEST(DeltaVisitorTests, ChangeOneField) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.inlineInt() = false;

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineInt", DeltaElemTag::MINIMAL)}));

  // test encoding IDs
  differingPaths.clear();
  result = RootDeltaVisitor::visit(
      nodeA,
      nodeB,
      DeltaVisitOptions(
          DeltaVisitMode::PARENTS,
          thrift_cow::DeltaVisitOrder::PARENTS_FIRST,
          true),
      processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/2", DeltaElemTag::MINIMAL)}));

  // test MINIMAL delta mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{std::make_pair("/inlineInt", DeltaElemTag::MINIMAL)}));

  // test FULL delta mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineInt", DeltaElemTag::MINIMAL)}));
}

TEST(DeltaVisitorTests, ChangeOneFieldInContainer) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.mapOfEnumToStruct()->at(TestEnum::THIRD).min() = 11;

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/3", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/3/min", DeltaElemTag::MINIMAL)}));

  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/mapOfEnumToStruct/3/min", DeltaElemTag::MINIMAL)}));
}

TEST(DeltaVisitorTests, SetOptional) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.optionalString() = "now I'm set";

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/optionalString", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/optionalString", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/optionalString", DeltaElemTag::MINIMAL)}));
}

TEST(DeltaVisitorTests, AddToMap) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;

  cfg::L4PortRange newOne;
  newOne.min() = 40;
  newOne.max() = 100;
  structB.mapOfEnumToStruct()->emplace(TestEnum::FIRST, std::move(newOne));

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1/min", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1/max", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              "/mapOfEnumToStruct/1/invert", DeltaElemTag::NOT_MINIMAL)}));
}

TEST(DeltaVisitorTests, UpdateMap) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;

  cfg::L4PortRange oldOne;
  oldOne.min() = 40;
  oldOne.max() = 100;
  oldOne.invert() = false;
  structA.mapOfEnumToStruct()->emplace(TestEnum::FIRST, std::move(oldOne));

  // update fields
  cfg::L4PortRange newOne;
  newOne.min() = 400;
  newOne.max() = 1000;
  structB.mapOfEnumToStruct()->emplace(TestEnum::FIRST, std::move(newOne));

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1/min", DeltaElemTag::MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1/max", DeltaElemTag::MINIMAL),
      }));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/mapOfEnumToStruct/1/min", DeltaElemTag::MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1/max", DeltaElemTag::MINIMAL)}));

  // Test encoding ids
  differingPaths.clear();
  result = RootDeltaVisitor::visit(
      nodeA,
      nodeB,
      DeltaVisitOptions(
          DeltaVisitMode::MINIMAL,
          thrift_cow::DeltaVisitOrder::PARENTS_FIRST,
          true),
      processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/15/1/1", DeltaElemTag::MINIMAL),
          std::make_pair("/15/1/2", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1/min", DeltaElemTag::MINIMAL),
          std::make_pair("/mapOfEnumToStruct/1/max", DeltaElemTag::MINIMAL),
      }));
}

TEST(DeltaVisitorTests, DeleteFromMap) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.mapOfEnumToStruct()->erase(TestEnum::THIRD);

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/3", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/mapOfEnumToStruct/3", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/3", DeltaElemTag::MINIMAL),
          std::make_pair("/mapOfEnumToStruct/3/min", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/mapOfEnumToStruct/3/max", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              "/mapOfEnumToStruct/3/invert", DeltaElemTag::NOT_MINIMAL)}));
}

TEST(DeltaVisitorTests, AddToList) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  cfg::L4PortRange newOne;
  newOne.min() = 40;
  newOne.max() = 100;
  structB.listOfStructs()->push_back(std::move(newOne));

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/listOfStructs", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/listOfStructs/0", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/listOfStructs/0", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/listOfStructs", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/listOfStructs/0", DeltaElemTag::MINIMAL),
          std::make_pair("/listOfStructs/0/min", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/listOfStructs/0/max", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              "/listOfStructs/0/invert", DeltaElemTag::NOT_MINIMAL)}));
}

TEST(DeltaVisitorTests, DeleteFromList) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  cfg::L4PortRange newOne;
  newOne.min() = 40;
  newOne.max() = 100;
  structB.listOfStructs()->push_back(std::move(newOne));

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeB, nodeA, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/listOfStructs", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/listOfStructs/0", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeB, nodeA, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/listOfStructs/0", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeB, nodeA, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/listOfStructs", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/listOfStructs/0", DeltaElemTag::MINIMAL),
          std::make_pair("/listOfStructs/0/min", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/listOfStructs/0/max", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              "/listOfStructs/0/invert", DeltaElemTag::NOT_MINIMAL)}));
}

TEST(DeltaVisitorTests, EditVariantField) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.inlineVariant()->inlineInt_ref() = 1000;

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL)}));

  // Test with ids
  differingPaths.clear();
  result = RootDeltaVisitor::visit(
      nodeA,
      nodeB,
      DeltaVisitOptions(
          DeltaVisitMode::FULL,
          thrift_cow::DeltaVisitOrder::PARENTS_FIRST,
          true),
      processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/21", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/21/2", DeltaElemTag::MINIMAL)}));
}

TEST(DeltaVisitorTests, SwitchVariantField) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.inlineVariant()->inlineBool_ref() = true;

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL),
          std::make_pair("/inlineVariant/inlineBool", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL),
          std::make_pair("/inlineVariant/inlineBool", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL),
          std::make_pair("/inlineVariant/inlineBool", DeltaElemTag::MINIMAL)}));
}

TEST(DeltaVisitorTests, SwitchVariantFieldToStruct) {
  auto structA = createSimpleTestStruct();

  cfg::L4PortRange newOne;
  newOne.min() = 40;
  newOne.max() = 100;

  auto structB = structA;
  structB.inlineVariant()->inlineStruct_ref() = std::move(newOne);

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PathTagSet differingPaths;
  auto processChange = [&](const std::vector<std::string>& path,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(std::make_pair("/" + folly::join('/', path), tag));
  };

  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL),
          std::make_pair(
              "/inlineVariant/inlineStruct", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL),
          std::make_pair(
              "/inlineVariant/inlineStruct", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(PathTagSet{
          std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
          std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL),
          std::make_pair("/inlineVariant/inlineStruct", DeltaElemTag::MINIMAL),
          std::make_pair(
              "/inlineVariant/inlineStruct/min", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              "/inlineVariant/inlineStruct/max", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              "/inlineVariant/inlineStruct/invert",
              DeltaElemTag::NOT_MINIMAL)}));
}

} // namespace facebook::fboss::thrift_cow::test

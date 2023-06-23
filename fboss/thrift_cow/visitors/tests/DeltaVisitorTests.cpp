// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/visitors/DeltaVisitor.h>
#include <thrift/lib/cpp2/reflection/folly_dynamic.h>
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

using folly::dynamic;

namespace {
using namespace facebook::fboss;

using PathTagSet = std::set<std::pair<std::string, thrift_cow::DeltaElemTag>>;

TestStruct createTestStruct() {
  dynamic testDyn = dynamic::object("inlineBool", true)("inlineInt", 54)(
      "inlineString",
      "testname")("optionalString", "bla")("inlineStruct", dynamic::object("min", 10)("max", 20))("inlineVariant", dynamic::object("inlineInt", 99))("mapOfEnumToStruct", dynamic::object("3", dynamic::object("min", 100)("max", 200)));

  return apache::thrift::from_dynamic<TestStruct>(
      testDyn, apache::thrift::dynamic_format::JSON_1);
}

} // namespace

TEST(DeltaVisitorTests, ChangeOneField) {
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();
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
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();
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
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();
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
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();
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
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();
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
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();
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
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();
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
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();
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
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();
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
}

TEST(DeltaVisitorTests, SwitchVariantField) {
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();
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
  using namespace facebook::fboss::thrift_cow;

  auto structA = createTestStruct();

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

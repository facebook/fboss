// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/visitors/DeltaVisitor.h>
#include <fboss/thrift_cow/visitors/tests/VisitorTestUtils.h>
#include "fboss/thrift_cow/nodes/Types.h"

using folly::dynamic;
using namespace testing;

namespace {
using namespace facebook::fboss;

using PathTagSet = std::set<std::pair<std::string, thrift_cow::DeltaElemTag>>;
} // namespace

namespace facebook::fboss::thrift_cow::test {

template <bool EnableHybridStorage>
struct TestParams {
  static constexpr auto hybridStorage = EnableHybridStorage;
};

using StorageTestTypes = ::testing::Types<TestParams<false>, TestParams<true>>;

template <typename TestParams>
class DeltaVisitorTests : public ::testing::Test {
 public:
  auto initNode(auto val) {
    using RootType = std::remove_cvref_t<decltype(val)>;
    return std::make_shared<ThriftStructNode<
        RootType,
        ThriftStructResolver<RootType, TestParams::hybridStorage>,
        TestParams::hybridStorage>>(val);
  }
  bool isHybridStorage() {
    return TestParams::hybridStorage;
  }
};

TYPED_TEST_SUITE(DeltaVisitorTests, StorageTestTypes);

TYPED_TEST(DeltaVisitorTests, ChangeOneField) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.inlineInt() = false;

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
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
      ::testing::ContainerEq(
          PathTagSet{
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
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/inlineInt", DeltaElemTag::MINIMAL)}));
}

TYPED_TEST(DeltaVisitorTests, ChangeOneFieldInContainer) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.mapOfEnumToStruct()->at(TestEnum::THIRD).min() = 11;

  PathTagSet expected;
  expected.emplace(std::make_pair("/", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL));
  if (this->isHybridStorage()) {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/3", DeltaElemTag::MINIMAL));
  } else {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/3", DeltaElemTag::NOT_MINIMAL));
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/3/min", DeltaElemTag::MINIMAL));
  }

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));

  differingPaths.clear();
  expected.clear();
  if (this->isHybridStorage()) {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/3", DeltaElemTag::MINIMAL));
  } else {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/3/min", DeltaElemTag::MINIMAL));
  }

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));
}

TYPED_TEST(DeltaVisitorTests, SetOptional) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.optionalString() = "now I'm set";

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/optionalString", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/optionalString", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/optionalString", DeltaElemTag::MINIMAL)}));
}

TYPED_TEST(DeltaVisitorTests, AddToMap) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;

  cfg::L4PortRange newOne;
  newOne.min() = 40;
  newOne.max() = 100;
  structB.mapOfEnumToStruct()->emplace(TestEnum::FIRST, std::move(newOne));

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet expected;
  expected.emplace(std::make_pair("/", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::MINIMAL));

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));

  // Test MINIMAL mode
  differingPaths.clear();
  expected.clear();
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::MINIMAL));

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));

  // Test FULL mode
  differingPaths.clear();
  expected.clear();
  expected.emplace(std::make_pair("/", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::MINIMAL));
  if (!this->isHybridStorage()) {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1/min", DeltaElemTag::NOT_MINIMAL));
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1/max", DeltaElemTag::NOT_MINIMAL));
    expected.emplace(
        std::make_pair(
            "/mapOfEnumToStruct/1/invert", DeltaElemTag::NOT_MINIMAL));
  }

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));
}

TYPED_TEST(DeltaVisitorTests, UpdateMap) {
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

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet expected;
  expected.emplace(std::make_pair("/", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL));
  if (this->isHybridStorage()) {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::MINIMAL));
  } else {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::NOT_MINIMAL));
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1/min", DeltaElemTag::MINIMAL));
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1/max", DeltaElemTag::MINIMAL));
  }

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));

  // Test MINIMAL mode
  differingPaths.clear();
  expected.clear();
  if (this->isHybridStorage()) {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::MINIMAL));
  } else {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1/min", DeltaElemTag::MINIMAL));
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1/max", DeltaElemTag::MINIMAL));
  }

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));

  // Test encoding ids
  differingPaths.clear();
  expected.clear();
  if (this->isHybridStorage()) {
    expected.emplace(std::make_pair("/15/1", DeltaElemTag::MINIMAL));
  } else {
    expected.emplace(std::make_pair("/15/1/1", DeltaElemTag::MINIMAL));
    expected.emplace(std::make_pair("/15/1/2", DeltaElemTag::MINIMAL));
  }
  result = RootDeltaVisitor::visit(
      nodeA,
      nodeB,
      DeltaVisitOptions(
          DeltaVisitMode::MINIMAL,
          thrift_cow::DeltaVisitOrder::PARENTS_FIRST,
          true),
      processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));

  // Test FULL mode
  differingPaths.clear();
  expected.clear();
  expected.emplace(std::make_pair("/", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL));
  if (this->isHybridStorage()) {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::MINIMAL));
  } else {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1", DeltaElemTag::NOT_MINIMAL));
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1/min", DeltaElemTag::MINIMAL));
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/1/max", DeltaElemTag::MINIMAL));
  }

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));
}

TYPED_TEST(DeltaVisitorTests, DeleteFromMap) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.mapOfEnumToStruct()->erase(TestEnum::THIRD);

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet expected;
  expected.emplace(std::make_pair("/", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct/3", DeltaElemTag::MINIMAL));

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));

  // Test MINIMAL mode
  differingPaths.clear();
  expected.clear();
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct/3", DeltaElemTag::MINIMAL));

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));

  // Test FULL mode
  differingPaths.clear();
  expected.clear();
  expected.emplace(std::make_pair("/", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct", DeltaElemTag::NOT_MINIMAL));
  expected.emplace(
      std::make_pair("/mapOfEnumToStruct/3", DeltaElemTag::MINIMAL));
  if (!this->isHybridStorage()) {
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/3/min", DeltaElemTag::NOT_MINIMAL));
    expected.emplace(
        std::make_pair("/mapOfEnumToStruct/3/max", DeltaElemTag::NOT_MINIMAL));
    expected.emplace(
        std::make_pair(
            "/mapOfEnumToStruct/3/invert", DeltaElemTag::NOT_MINIMAL));
  }

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expected));
}

TYPED_TEST(DeltaVisitorTests, AddToList) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  cfg::L4PortRange newOne;
  newOne.min() = 40;
  newOne.max() = 100;
  structB.listOfStructs()->push_back(std::move(newOne));

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
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
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/listOfStructs/0", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/listOfStructs", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/listOfStructs/0", DeltaElemTag::MINIMAL),
              std::make_pair("/listOfStructs/0/min", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/listOfStructs/0/max", DeltaElemTag::NOT_MINIMAL),
              std::make_pair(
                  "/listOfStructs/0/invert", DeltaElemTag::NOT_MINIMAL)}));
}

TYPED_TEST(DeltaVisitorTests, DeleteFromList) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  cfg::L4PortRange newOne;
  newOne.min() = 40;
  newOne.max() = 100;
  structB.listOfStructs()->push_back(std::move(newOne));

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeB, nodeA, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
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
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/listOfStructs/0", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeB, nodeA, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/listOfStructs", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/listOfStructs/0", DeltaElemTag::MINIMAL),
              std::make_pair("/listOfStructs/0/min", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/listOfStructs/0/max", DeltaElemTag::NOT_MINIMAL),
              std::make_pair(
                  "/listOfStructs/0/invert", DeltaElemTag::NOT_MINIMAL)}));
}

TYPED_TEST(DeltaVisitorTests, EditVariantField) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.inlineVariant()->inlineInt() = 1000;

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
              std::make_pair(
                  "/inlineVariant/inlineInt", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{std::make_pair(
              "/inlineVariant/inlineInt", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
              std::make_pair(
                  "/inlineVariant/inlineInt", DeltaElemTag::MINIMAL)}));

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
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/21", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/21/2", DeltaElemTag::MINIMAL)}));
}

TYPED_TEST(DeltaVisitorTests, SwitchVariantField) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;
  structB.inlineVariant()->inlineBool() = true;

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL),
              std::make_pair(
                  "/inlineVariant/inlineBool", DeltaElemTag::MINIMAL)}));

  // Test MINIMAL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL),
              std::make_pair(
                  "/inlineVariant/inlineBool", DeltaElemTag::MINIMAL)}));

  // Test FULL mode
  differingPaths.clear();

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL),
              std::make_pair(
                  "/inlineVariant/inlineBool", DeltaElemTag::MINIMAL)}));
}

TYPED_TEST(DeltaVisitorTests, SwitchVariantFieldToStruct) {
  auto structA = createSimpleTestStruct();

  cfg::L4PortRange newOne;
  newOne.min() = 40;
  newOne.max() = 100;

  auto structB = structA;
  structB.inlineVariant()->inlineStruct() = std::move(newOne);

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
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
      ::testing::ContainerEq(
          PathTagSet{
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
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/inlineVariant", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/inlineVariant/inlineInt", DeltaElemTag::MINIMAL),
              std::make_pair(
                  "/inlineVariant/inlineStruct", DeltaElemTag::MINIMAL),
              std::make_pair(
                  "/inlineVariant/inlineStruct/min", DeltaElemTag::NOT_MINIMAL),
              std::make_pair(
                  "/inlineVariant/inlineStruct/max", DeltaElemTag::NOT_MINIMAL),
              std::make_pair(
                  "/inlineVariant/inlineStruct/invert",
                  DeltaElemTag::NOT_MINIMAL)}));
}

TYPED_TEST(DeltaVisitorTests, RecursiveStruct) {
  // Build a self-referential RecursiveStruct hierarchy under
  // TestStruct.recursiveMember, then change a leaf two levels of recursion
  // deep:
  //   recursiveMember[0].children[1].children[0].simpleMember.min : 22 -> 33
  auto structA = createSimpleTestStruct();
  structA.recursiveMember()->push_back(makeRecursiveStruct());

  auto structB = structA;
  structB.recursiveMember()
      ->at(0)
      .children()
      ->at(1)
      .children()
      ->at(0)
      .simpleMember()
      ->min() = 33;

  // The differing leaf, and the parent chain leading to it. RecursiveStruct is
  // a pure COW struct and recursiveMember is an unannotated list<struct>. Just
  // like the existing listOfStructs tests (see AddToList), such members are
  // traversed as COW containers under *both* storage modes, so the delta
  // descends to the changed primitive leaf regardless of isHybridStorage().
  // (Only hybrid-native maps or AllowSkipThriftCow members collapse into opaque
  // leaves.)
  const std::string leafPath =
      "/recursiveMember/0/children/1/children/0/simpleMember/min";

  PathTagSet expectedParents{
      std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/recursiveMember", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/recursiveMember/0", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/recursiveMember/0/children", DeltaElemTag::NOT_MINIMAL),
      std::make_pair(
          "/recursiveMember/0/children/1", DeltaElemTag::NOT_MINIMAL),
      std::make_pair(
          "/recursiveMember/0/children/1/children", DeltaElemTag::NOT_MINIMAL),
      std::make_pair(
          "/recursiveMember/0/children/1/children/0",
          DeltaElemTag::NOT_MINIMAL),
      std::make_pair(
          "/recursiveMember/0/children/1/children/0/simpleMember",
          DeltaElemTag::NOT_MINIMAL),
      std::make_pair(leafPath, DeltaElemTag::MINIMAL)};

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expectedParents));

  // id-token form: same shape, but every path element is the field id / index.
  // Field ids: recursiveMember=35, children=3, simpleMember=2, min=1.
  differingPaths.clear();
  const PathTagSet expectedIdParents{
      std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/35", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/35/0", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/35/0/3", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/35/0/3/1", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/35/0/3/1/3", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/35/0/3/1/3/0", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/35/0/3/1/3/0/2", DeltaElemTag::NOT_MINIMAL),
      std::make_pair("/35/0/3/1/3/0/2/1", DeltaElemTag::MINIMAL)};

  result = RootDeltaVisitor::visit(
      nodeA,
      nodeB,
      DeltaVisitOptions(
          DeltaVisitMode::PARENTS,
          thrift_cow::DeltaVisitOrder::PARENTS_FIRST,
          true),
      processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expectedIdParents));

  // MINIMAL mode reports only the minimal differing unit: the changed leaf.
  differingPaths.clear();
  const PathTagSet expectedMinimal{
      std::make_pair(leafPath, DeltaElemTag::MINIMAL)};

  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expectedMinimal));

  // FULL mode: the changed value is a primitive leaf, so (as in ChangeOneField)
  // there is no additional subtree to expand -- FULL matches the PARENTS chain.
  differingPaths.clear();
  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(differingPaths, ::testing::ContainerEq(expectedParents));
}

TYPED_TEST(DeltaVisitorTests, AddToRecursiveList) {
  // Appending a child to a recursive list<RecursiveStruct> reports the new
  // element as the minimal delta, with the parent chain above it.
  auto structA = createSimpleTestStruct();
  structA.recursiveMember()->push_back(makeRecursiveStruct());

  auto structB = structA;
  RecursiveStruct added;
  added.name() = "addedChild";
  structB.recursiveMember()->at(0).children()->push_back(std::move(added));

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  // makeRecursiveStruct() populates children[0] and children[1]; the appended
  // element is the new index 2.
  const std::string addedPath = "/recursiveMember/0/children/2";
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/recursiveMember", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/recursiveMember/0", DeltaElemTag::NOT_MINIMAL),
              std::make_pair(
                  "/recursiveMember/0/children", DeltaElemTag::NOT_MINIMAL),
              std::make_pair(addedPath, DeltaElemTag::MINIMAL)}));

  // MINIMAL mode reports only the newly added element.
  differingPaths.clear();
  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{std::make_pair(addedPath, DeltaElemTag::MINIMAL)}));

  // FULL mode: the added element is a whole recursive subtree, so FULL must
  // expand it into every descendant leaf (the added node itself is MINIMAL, its
  // descendants NOT_MINIMAL -- mirroring AddToList). Assert a superset of the
  // added element's leaves rather than the exact set, to avoid over-fitting to
  // the exact intermediate-node emission.
  differingPaths.clear();
  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::IsSupersetOf({
          std::make_pair(addedPath, DeltaElemTag::MINIMAL),
          std::make_pair(addedPath + "/name", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              addedPath + "/simpleMember/min", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              addedPath + "/simpleMember/max", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              addedPath + "/simpleMember/invert", DeltaElemTag::NOT_MINIMAL),
      }));
}

TYPED_TEST(DeltaVisitorTests, RemoveFromRecursiveList) {
  // Deleting an element from a recursive list<RecursiveStruct> reports the
  // removed element (the last index) as the minimal delta. This exercises the
  // index-shift path against a self-referential list.
  auto structA = createSimpleTestStruct();
  structA.recursiveMember()->push_back(makeRecursiveStruct());

  auto structB = structA;
  // makeRecursiveStruct() leaves children with two elements; drop the last.
  structB.recursiveMember()->at(0).children()->pop_back();

  auto nodeA = this->initNode(structA);
  auto nodeB = this->initNode(structB);

  PathTagSet differingPaths;
  auto processChange = [&](SimpleTraverseHelper& traverser,
                           auto&& /*oldValue*/,
                           auto&& /*newValue*/,
                           auto&& tag) {
    differingPaths.emplace(
        std::make_pair("/" + folly::join('/', traverser.path()), tag));
  };

  thrift_cow::SimpleTraverseHelper traverser;
  auto result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::PARENTS), processChange);
  EXPECT_EQ(result, true);
  const std::string removedPath = "/recursiveMember/0/children/1";
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{
              std::make_pair("/", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/recursiveMember", DeltaElemTag::NOT_MINIMAL),
              std::make_pair("/recursiveMember/0", DeltaElemTag::NOT_MINIMAL),
              std::make_pair(
                  "/recursiveMember/0/children", DeltaElemTag::NOT_MINIMAL),
              std::make_pair(removedPath, DeltaElemTag::MINIMAL)}));

  // MINIMAL mode reports only the removed element.
  differingPaths.clear();
  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::MINIMAL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          PathTagSet{std::make_pair(removedPath, DeltaElemTag::MINIMAL)}));

  // FULL mode: the removed element is a deep recursive subtree (it has its own
  // children), so FULL must expand it all the way down -- including the
  // grandchild leaf two levels of recursion deep. Superset assertion, as in
  // AddToRecursiveList.
  differingPaths.clear();
  result = RootDeltaVisitor::visit(
      nodeA, nodeB, DeltaVisitOptions(DeltaVisitMode::FULL), processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::IsSupersetOf({
          std::make_pair(removedPath, DeltaElemTag::MINIMAL),
          std::make_pair(removedPath + "/name", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              removedPath + "/simpleMember/min", DeltaElemTag::NOT_MINIMAL),
          std::make_pair(
              removedPath + "/children/0/simpleMember/min",
              DeltaElemTag::NOT_MINIMAL),
      }));
}

} // namespace facebook::fboss::thrift_cow::test

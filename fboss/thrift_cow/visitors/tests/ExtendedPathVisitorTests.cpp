// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fmt/format.h>
#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/visitors/ExtendedPathVisitor.h>
#include <fboss/thrift_cow/visitors/tests/VisitorTestUtils.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/thrift_cow/nodes/Types.h"

using folly::dynamic;
using namespace facebook::fboss;
using namespace facebook::fboss::fsdb;
using namespace facebook::fboss::thrift_cow;
using k = facebook::fboss::test_tags::strings;
using TestStructMembers = apache::thrift::reflect_struct<TestStruct>::member;

namespace {

TestStruct createTestStruct() {
  dynamic testDyn = dynamic::object("inlineBool", true)("inlineInt", 54)(
      "inlineString", "testname")("optionalString", "bla")(
      "inlineStruct", dynamic::object("min", 10)("max", 20))(
      "inlineVariant", dynamic::object("inlineInt", 99))(
      "mapOfEnumToStruct",
      dynamic::object("3", dynamic::object("min", 100)("max", 200)))(
      "mapOfStringToI32", dynamic::object)(
      "listOfPrimitives", dynamic::array())("setOfI32", dynamic::array())(
      "mapOfI32ToListOfStructs", dynamic::object())(
      "hybridMapOfMap", dynamic::object())(
      "hybridStruct",
      dynamic::object("strMap", dynamic::object())(
          "structMap", dynamic::object()));

  for (int i = 0; i <= 20; ++i) {
    testDyn["mapOfStringToI32"][fmt::format("test{}", i)] = i;
    testDyn["listOfPrimitives"].push_back(i);
    testDyn["setOfI32"].push_back(i);
    testDyn["mapOfI32ToListOfStructs"][folly::to<std::string>(i)] =
        dynamic::array(
            dynamic::object("min", 1)("max", 2),
            dynamic::object("min", 3)("max", 4));
    dynamic innerMap = dynamic::object();
    innerMap[i * 10] = i * 100;
    testDyn["hybridMapOfMap"][i] = std::move(innerMap);
    testDyn["hybridStruct"]["strMap"][fmt::format("test{}", i)] = i;
    testDyn["hybridStruct"]["structMap"][fmt::format("test{}", i)] =
        dynamic::object("min", i)("max", i + 10);
  }

  return facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
}
} // namespace

TEST(ExtendedPathVisitorTests, AccessFieldSimple) {
  auto structA = createTestStruct();

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  folly::dynamic dyn;
  std::vector<std::string> recvPath;
  auto processPath = [&dyn, &recvPath](auto&& path, auto&& node) {
    dyn = node.toFollyDynamic();
    recvPath = path;
  };

  auto path = ext_path_builder::raw("inlineInt").get();
  ExtPathVisitorOptions options;
  RootExtendedPathVisitor::visit(
      *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
  EXPECT_EQ(dyn, 54);

  dyn = dynamic::object;
  path = ext_path_builder::raw(TestStructMembers::inlineInt::id::value).get();
  options.outputIdPaths = true;
  RootExtendedPathVisitor::visit(
      *nodeA,
      path.path()->begin(),
      path.path()->end(),
      options,
      std::move(processPath));
  EXPECT_EQ(dyn, 54);
  std::vector<std::string> expectPath = {
      folly::to<std::string>(TestStructMembers::inlineInt::id::value)};
  EXPECT_EQ(recvPath, expectPath);
}

TEST(ExtendedPathVisitorTests, AccessFieldInContainer) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  cfg::L4PortRange got;
  auto processPath = [&got](auto&&, auto&& node) {
    if constexpr (std::is_assignable_v<
                      decltype(got),
                      decltype(node.toThrift())>) {
      got = node.toThrift();
    }
  };

  auto path = ext_path_builder::raw("mapOfEnumToStruct").raw("3").get();
  ExtPathVisitorOptions options;
  RootExtendedPathVisitor::visit(
      *nodeA,
      path.path()->begin(),
      path.path()->end(),
      options,
      std::move(processPath));
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);
}

TEST(ExtendedPathVisitorTests, AccessOptional) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  std::string got;
  auto processPath = [&got](auto&&, auto&& node) {
    got = node.toFollyDynamic().asString();
  };

  auto path = ext_path_builder::raw("optionalString").get();
  ExtPathVisitorOptions options;
  RootExtendedPathVisitor::visit(
      *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
  EXPECT_EQ(got, "bla");
  structA.optionalString().reset();
  nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  got.clear();
  RootExtendedPathVisitor::visit(
      *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
  EXPECT_TRUE(got.empty());
}

template <bool EnableHybridStorage>
struct TestParams {
  static constexpr auto hybridStorage = EnableHybridStorage;
};

using StorageTestTypes = ::testing::Types<TestParams<false>, TestParams<true>>;

template <typename TestParams>
class ExtendedPathVisitorTests : public ::testing::Test {
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

TYPED_TEST_SUITE(ExtendedPathVisitorTests, StorageTestTypes);

TYPED_TEST(ExtendedPathVisitorTests, AccessRegexMap) {
  auto structA = createTestStruct();
  auto nodeA = this->initNode(structA);

  std::set<std::pair<std::vector<std::string>, folly::dynamic>> visited;
  auto processPath = [&visited](auto&& path, auto&& node) {
    visited.emplace(std::make_pair(path, node.toFollyDynamic()));
  };

  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
      {{"mapOfStringToI32", "test1"}, 1},
      {{"mapOfStringToI32", "test10"}, 10},
      {{"mapOfStringToI32", "test11"}, 11},
      {{"mapOfStringToI32", "test12"}, 12},
      {{"mapOfStringToI32", "test13"}, 13},
      {{"mapOfStringToI32", "test14"}, 14},
      {{"mapOfStringToI32", "test15"}, 15},
      {{"mapOfStringToI32", "test16"}, 16},
      {{"mapOfStringToI32", "test17"}, 17},
      {{"mapOfStringToI32", "test18"}, 18},
      {{"mapOfStringToI32", "test19"}, 19},
  };

  ExtPathVisitorOptions options;
  RootExtendedPathVisitor::visit(
      *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
  EXPECT_THAT(visited, ::testing::ContainerEq(expected));

  auto path2 = ext_path_builder::raw("mapOfI32ToListOfStructs")
                   .regex("1.*")
                   .regex(".*")
                   .raw("min")
                   .get();
  std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected2 = {
      {{"mapOfI32ToListOfStructs", "1", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "1", "1", "min"}, 3},
      {{"mapOfI32ToListOfStructs", "10", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "10", "1", "min"}, 3},
      {{"mapOfI32ToListOfStructs", "11", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "11", "1", "min"}, 3},
      {{"mapOfI32ToListOfStructs", "12", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "12", "1", "min"}, 3},
      {{"mapOfI32ToListOfStructs", "13", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "13", "1", "min"}, 3},
      {{"mapOfI32ToListOfStructs", "14", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "14", "1", "min"}, 3},
      {{"mapOfI32ToListOfStructs", "15", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "15", "1", "min"}, 3},
      {{"mapOfI32ToListOfStructs", "16", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "16", "1", "min"}, 3},
      {{"mapOfI32ToListOfStructs", "17", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "17", "1", "min"}, 3},
      {{"mapOfI32ToListOfStructs", "18", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "18", "1", "min"}, 3},
      {{"mapOfI32ToListOfStructs", "19", "0", "min"}, 1},
      {{"mapOfI32ToListOfStructs", "19", "1", "min"}, 3},
  };

  visited.clear();
  RootExtendedPathVisitor::visit(
      *nodeA, path2.path()->begin(), path2.path()->end(), options, processPath);
  EXPECT_THAT(visited, ::testing::ContainerEq(expected2));
}

TYPED_TEST(ExtendedPathVisitorTests, AccessAnyMap) {
  auto structA = createTestStruct();
  auto nodeA = this->initNode(structA);

  std::set<std::pair<std::vector<std::string>, folly::dynamic>> visited;
  auto processPath = [&visited](auto&& path, auto&& node) {
    visited.emplace(std::make_pair(path, node.toFollyDynamic()));
  };

  auto path = ext_path_builder::raw("mapOfStringToI32").any().get();
  std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
      {{"mapOfStringToI32", "test0"}, 0},
      {{"mapOfStringToI32", "test1"}, 1},
      {{"mapOfStringToI32", "test2"}, 2},
      {{"mapOfStringToI32", "test3"}, 3},
      {{"mapOfStringToI32", "test4"}, 4},
      {{"mapOfStringToI32", "test5"}, 5},
      {{"mapOfStringToI32", "test6"}, 6},
      {{"mapOfStringToI32", "test7"}, 7},
      {{"mapOfStringToI32", "test8"}, 8},
      {{"mapOfStringToI32", "test9"}, 9},
      {{"mapOfStringToI32", "test10"}, 10},
      {{"mapOfStringToI32", "test11"}, 11},
      {{"mapOfStringToI32", "test12"}, 12},
      {{"mapOfStringToI32", "test13"}, 13},
      {{"mapOfStringToI32", "test14"}, 14},
      {{"mapOfStringToI32", "test15"}, 15},
      {{"mapOfStringToI32", "test16"}, 16},
      {{"mapOfStringToI32", "test17"}, 17},
      {{"mapOfStringToI32", "test18"}, 18},
      {{"mapOfStringToI32", "test19"}, 19},
      {{"mapOfStringToI32", "test20"}, 20},
  };

  ExtPathVisitorOptions options;
  RootExtendedPathVisitor::visit(
      *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
  EXPECT_THAT(visited, ::testing::ContainerEq(expected));
}

TYPED_TEST(ExtendedPathVisitorTests, AccessDeepMap) {
  auto structA = createTestStruct();
  auto nodeA = this->initNode(structA);

  std::set<std::pair<std::vector<std::string>, folly::dynamic>> visited;
  auto processPath = [&visited](auto&& path, auto&& node) {
    visited.emplace(std::make_pair(path, node.toFollyDynamic()));
  };

  auto path = ext_path_builder::raw("hybridMapOfMap").any().any().get();
  std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
      {{"hybridMapOfMap", "0", "0"}, 0},
      {{"hybridMapOfMap", "1", "10"}, 100},
      {{"hybridMapOfMap", "2", "20"}, 200},
      {{"hybridMapOfMap", "3", "30"}, 300},
      {{"hybridMapOfMap", "4", "40"}, 400},
      {{"hybridMapOfMap", "5", "50"}, 500},
      {{"hybridMapOfMap", "6", "60"}, 600},
      {{"hybridMapOfMap", "7", "70"}, 700},
      {{"hybridMapOfMap", "8", "80"}, 800},
      {{"hybridMapOfMap", "9", "90"}, 900},
      {{"hybridMapOfMap", "10", "100"}, 1000},
      {{"hybridMapOfMap", "11", "110"}, 1100},
      {{"hybridMapOfMap", "12", "120"}, 1200},
      {{"hybridMapOfMap", "13", "130"}, 1300},
      {{"hybridMapOfMap", "14", "140"}, 1400},
      {{"hybridMapOfMap", "15", "150"}, 1500},
      {{"hybridMapOfMap", "16", "160"}, 1600},
      {{"hybridMapOfMap", "17", "170"}, 1700},
      {{"hybridMapOfMap", "18", "180"}, 1800},
      {{"hybridMapOfMap", "19", "190"}, 1900},
      {{"hybridMapOfMap", "20", "200"}, 2000},

  };

  ExtPathVisitorOptions options;
  RootExtendedPathVisitor::visit(
      *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
  EXPECT_THAT(visited, ::testing::ContainerEq(expected));
}

TYPED_TEST(ExtendedPathVisitorTests, AccessRegexList) {
  auto structA = createTestStruct();
  auto nodeA = this->initNode(structA);
  std::set<std::pair<std::vector<std::string>, folly::dynamic>> visited;
  auto processPath = [&visited](auto&& path, auto&& node) {
    visited.emplace(std::make_pair(path, node.toFollyDynamic()));
  };

  auto path = ext_path_builder::raw("listOfPrimitives").regex("1.*").get();
  std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
      {{"listOfPrimitives", "1"}, 1},
      {{"listOfPrimitives", "10"}, 10},
      {{"listOfPrimitives", "11"}, 11},
      {{"listOfPrimitives", "12"}, 12},
      {{"listOfPrimitives", "13"}, 13},
      {{"listOfPrimitives", "14"}, 14},
      {{"listOfPrimitives", "15"}, 15},
      {{"listOfPrimitives", "16"}, 16},
      {{"listOfPrimitives", "17"}, 17},
      {{"listOfPrimitives", "18"}, 18},
      {{"listOfPrimitives", "19"}, 19},
  };

  ExtPathVisitorOptions options;
  RootExtendedPathVisitor::visit(
      *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
  EXPECT_THAT(visited, ::testing::ContainerEq(expected));
}

TYPED_TEST(ExtendedPathVisitorTests, AccessAnyList) {
  auto structA = createTestStruct();
  auto nodeA = this->initNode(structA);

  std::set<std::pair<std::vector<std::string>, folly::dynamic>> visited;
  auto processPath = [&visited](auto&& path, auto&& node) {
    visited.emplace(std::make_pair(path, node.toFollyDynamic()));
  };

  auto path = ext_path_builder::raw("listOfPrimitives").any().get();
  std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
      {{"listOfPrimitives", "0"}, 0},   {{"listOfPrimitives", "1"}, 1},
      {{"listOfPrimitives", "2"}, 2},   {{"listOfPrimitives", "3"}, 3},
      {{"listOfPrimitives", "4"}, 4},   {{"listOfPrimitives", "5"}, 5},
      {{"listOfPrimitives", "6"}, 6},   {{"listOfPrimitives", "7"}, 7},
      {{"listOfPrimitives", "8"}, 8},   {{"listOfPrimitives", "9"}, 9},
      {{"listOfPrimitives", "10"}, 10}, {{"listOfPrimitives", "11"}, 11},
      {{"listOfPrimitives", "12"}, 12}, {{"listOfPrimitives", "13"}, 13},
      {{"listOfPrimitives", "14"}, 14}, {{"listOfPrimitives", "15"}, 15},
      {{"listOfPrimitives", "16"}, 16}, {{"listOfPrimitives", "17"}, 17},
      {{"listOfPrimitives", "18"}, 18}, {{"listOfPrimitives", "19"}, 19},
      {{"listOfPrimitives", "20"}, 20},
  };

  ExtPathVisitorOptions options;
  RootExtendedPathVisitor::visit(
      *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
  EXPECT_THAT(visited, ::testing::ContainerEq(expected));
}

TEST(ExtendedPathVisitorTests, AccessRegexSet) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  std::set<std::pair<std::vector<std::string>, folly::dynamic>> visited;
  auto processPath = [&visited](auto&& path, auto&& node) {
    visited.emplace(std::make_pair(path, node.toFollyDynamic()));
  };

  auto path = ext_path_builder::raw("setOfI32").regex("1.*").get();
  std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
      {{"setOfI32", "1"}, 1},
      {{"setOfI32", "10"}, 10},
      {{"setOfI32", "11"}, 11},
      {{"setOfI32", "12"}, 12},
      {{"setOfI32", "13"}, 13},
      {{"setOfI32", "14"}, 14},
      {{"setOfI32", "15"}, 15},
      {{"setOfI32", "16"}, 16},
      {{"setOfI32", "17"}, 17},
      {{"setOfI32", "18"}, 18},
      {{"setOfI32", "19"}, 19},
  };

  ExtPathVisitorOptions options;
  RootExtendedPathVisitor::visit(
      *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
  EXPECT_THAT(visited, ::testing::ContainerEq(expected));
}

TEST(ExtendedPathVisitorTests, AccessAnySet) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  std::set<std::pair<std::vector<std::string>, folly::dynamic>> visited;
  auto processPath = [&visited](auto&& path, auto&& node) {
    visited.emplace(std::make_pair(path, node.toFollyDynamic()));
  };

  auto path = ext_path_builder::raw("setOfI32").any().get();
  std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
      {{"setOfI32", "0"}, 0},   {{"setOfI32", "1"}, 1},
      {{"setOfI32", "2"}, 2},   {{"setOfI32", "3"}, 3},
      {{"setOfI32", "4"}, 4},   {{"setOfI32", "5"}, 5},
      {{"setOfI32", "6"}, 6},   {{"setOfI32", "7"}, 7},
      {{"setOfI32", "8"}, 8},   {{"setOfI32", "9"}, 9},
      {{"setOfI32", "10"}, 10}, {{"setOfI32", "11"}, 11},
      {{"setOfI32", "12"}, 12}, {{"setOfI32", "13"}, 13},
      {{"setOfI32", "14"}, 14}, {{"setOfI32", "15"}, 15},
      {{"setOfI32", "16"}, 16}, {{"setOfI32", "17"}, 17},
      {{"setOfI32", "18"}, 18}, {{"setOfI32", "19"}, 19},
      {{"setOfI32", "20"}, 20},
  };

  ExtPathVisitorOptions options;
  RootExtendedPathVisitor::visit(
      *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
  EXPECT_THAT(visited, ::testing::ContainerEq(expected));
}

TYPED_TEST(ExtendedPathVisitorTests, HybridStructAccess) {
  auto structA = createTestStruct();
  auto nodeA = this->initNode(structA);

  std::set<std::pair<std::vector<std::string>, folly::dynamic>> visited;
  auto processPath = [&visited](auto&& path, auto&& node) {
    visited.emplace(std::make_pair(path, node.toFollyDynamic()));
  };
  // hybridStruct/strMap/.*
  {
    auto path = ext_path_builder::raw("hybridStruct").raw("strMap").any().get();
    std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
        {{"hybridStruct", "strMap", "test0"}, 0},
        {{"hybridStruct", "strMap", "test1"}, 1},
        {{"hybridStruct", "strMap", "test2"}, 2},
        {{"hybridStruct", "strMap", "test3"}, 3},
        {{"hybridStruct", "strMap", "test4"}, 4},
        {{"hybridStruct", "strMap", "test5"}, 5},
        {{"hybridStruct", "strMap", "test6"}, 6},
        {{"hybridStruct", "strMap", "test7"}, 7},
        {{"hybridStruct", "strMap", "test8"}, 8},
        {{"hybridStruct", "strMap", "test9"}, 9},
        {{"hybridStruct", "strMap", "test10"}, 10},
        {{"hybridStruct", "strMap", "test11"}, 11},
        {{"hybridStruct", "strMap", "test12"}, 12},
        {{"hybridStruct", "strMap", "test13"}, 13},
        {{"hybridStruct", "strMap", "test14"}, 14},
        {{"hybridStruct", "strMap", "test15"}, 15},
        {{"hybridStruct", "strMap", "test16"}, 16},
        {{"hybridStruct", "strMap", "test17"}, 17},
        {{"hybridStruct", "strMap", "test18"}, 18},
        {{"hybridStruct", "strMap", "test19"}, 19},
        {{"hybridStruct", "strMap", "test20"}, 20},
    };

    ExtPathVisitorOptions options;
    RootExtendedPathVisitor::visit(
        *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
    EXPECT_THAT(visited, ::testing::ContainerEq(expected));
  }
  // hybridStruct/strMap/.*
  {
    auto path = ext_path_builder::raw("hybridStruct")
                    .raw("structMap")
                    .any()
                    .raw("min")
                    .get();
    std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
        {{"hybridStruct", "structMap", "test0", "min"}, 0},
        {{"hybridStruct", "structMap", "test1", "min"}, 1},
        {{"hybridStruct", "structMap", "test2", "min"}, 2},
        {{"hybridStruct", "structMap", "test3", "min"}, 3},
        {{"hybridStruct", "structMap", "test4", "min"}, 4},
        {{"hybridStruct", "structMap", "test5", "min"}, 5},
        {{"hybridStruct", "structMap", "test6", "min"}, 6},
        {{"hybridStruct", "structMap", "test7", "min"}, 7},
        {{"hybridStruct", "structMap", "test8", "min"}, 8},
        {{"hybridStruct", "structMap", "test9", "min"}, 9},
        {{"hybridStruct", "structMap", "test10", "min"}, 10},
        {{"hybridStruct", "structMap", "test11", "min"}, 11},
        {{"hybridStruct", "structMap", "test12", "min"}, 12},
        {{"hybridStruct", "structMap", "test13", "min"}, 13},
        {{"hybridStruct", "structMap", "test14", "min"}, 14},
        {{"hybridStruct", "structMap", "test15", "min"}, 15},
        {{"hybridStruct", "structMap", "test16", "min"}, 16},
        {{"hybridStruct", "structMap", "test17", "min"}, 17},
        {{"hybridStruct", "structMap", "test18", "min"}, 18},
        {{"hybridStruct", "structMap", "test19", "min"}, 19},
        {{"hybridStruct", "structMap", "test20", "min"}, 20},
    };

    ExtPathVisitorOptions options;
    visited.clear();
    RootExtendedPathVisitor::visit(
        *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
    EXPECT_THAT(visited, ::testing::ContainerEq(expected));
  }
}

TYPED_TEST(ExtendedPathVisitorTests, HybridMapAccess) {
  auto structA = createTestStruct();
  auto nodeA = this->initNode(structA);

  std::set<std::pair<std::vector<std::string>, folly::dynamic>> visited;
  auto processPath = [&visited](auto&& path, auto&& node) {
    visited.emplace(std::make_pair(path, node.toFollyDynamic()));
  };
  // hybridStruct
  {
    folly::dynamic expectedFollyDynamicValue;
    facebook::thrift::to_dynamic(
        expectedFollyDynamicValue,
        structA.hybridStruct().value(),
        facebook::thrift::dynamic_format::JSON_1);
    auto path = ext_path_builder::raw("hybridStruct").get();
    std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
        {{"hybridStruct"}, expectedFollyDynamicValue}};

    ExtPathVisitorOptions options;
    RootExtendedPathVisitor::visit(
        *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
    EXPECT_THAT(visited, ::testing::ContainerEq(expected));
  }
}

TYPED_TEST(ExtendedPathVisitorTests, HybridMapRegexAccess) {
  TestStruct structA = createSimpleTestStruct();
  auto expectedValue = *structA.hybridMapOfI32ToStruct();

  ParentTestStruct root;
  root.mapOfI32ToMapOfStruct() = {{1, {{"key1", std::move(structA)}}}};
  auto nodeA = this->initNode(root);

  std::set<std::pair<std::vector<std::string>, folly::dynamic>> visited;
  auto processPath = [&visited](auto&& path, auto&& node) {
    visited.emplace(std::make_pair(path, node.toFollyDynamic()));
  };
  // regex path ending in hybrid map
  {
    EXPECT_TRUE(expectedValue.contains(20));
    folly::dynamic expectedFollyDynamicValue;
    facebook::thrift::to_dynamic(
        expectedFollyDynamicValue,
        expectedValue,
        facebook::thrift::dynamic_format::JSON_1);
    auto path = ext_path_builder::raw("mapOfI32ToMapOfStruct")
                    .raw("1")
                    .any()
                    .raw("hybridMapOfI32ToStruct")
                    .get();
    std::set<std::pair<std::vector<std::string>, folly::dynamic>> expected = {
        {{"mapOfI32ToMapOfStruct", "1", "key1", "hybridMapOfI32ToStruct"},
         expectedFollyDynamicValue}};

    ExtPathVisitorOptions options;
    RootExtendedPathVisitor::visit(
        *nodeA, path.path()->begin(), path.path()->end(), options, processPath);
    EXPECT_THAT(visited, ::testing::ContainerEq(expected));
  }
}

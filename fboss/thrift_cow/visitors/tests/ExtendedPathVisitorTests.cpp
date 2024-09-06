// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fmt/format.h>
#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/visitors/ExtendedPathVisitor.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

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
      "mapOfI32ToListOfStructs", dynamic::object());

  for (int i = 0; i <= 20; ++i) {
    testDyn["mapOfStringToI32"][fmt::format("test{}", i)] = i;
    testDyn["listOfPrimitives"].push_back(i);
    testDyn["setOfI32"].push_back(i);
    testDyn["mapOfI32ToListOfStructs"][folly::to<std::string>(i)] =
        dynamic::array(
            dynamic::object("min", 1)("max", 2),
            dynamic::object("min", 3)("max", 4));
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

TEST(ExtendedPathVisitorTests, AccessRegexMap) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

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

TEST(ExtendedPathVisitorTests, AccessAnyMap) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

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

TEST(ExtendedPathVisitorTests, AccessRegexList) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

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

TEST(ExtendedPathVisitorTests, AccessAnyList) {
  auto structA = createTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

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

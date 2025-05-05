// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/visitors/RecurseVisitor.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

using folly::dynamic;
using namespace facebook::fboss;
using namespace facebook::fboss::thrift_cow;
using k = facebook::fboss::test_tags::strings;
using namespace testing;

namespace {

folly::dynamic createTestDynamic() {
  return dynamic::object("inlineBool", true)("inlineInt", 54)(
      "inlineString", "testname")("optionalString", "bla")(
      "inlineStruct", dynamic::object("min", 10)("max", 20)("invert", false))(
      "inlineVariant", dynamic::object("inlineInt", 99))(
      "mapOfEnumToStruct",
      dynamic::object(
          3, dynamic::object("min", 100)("max", 200)("invert", false)))(
      "listOfListOfPrimitives", dynamic::array())(
      "listOfListOfStructs", dynamic::array())(
      "listOfPrimitives", dynamic::array())("listOfStructs", dynamic::array())(
      "mapOfEnumToI32", dynamic::object())("mapOfI32ToI32", dynamic::object())(
      "mapOfI32ToListOfStructs", dynamic::object())(
      "mapOfI32ToSetOfString", dynamic::object())(
      "mapOfI32ToStruct", dynamic::object())(
      "mapOfStringToI32", dynamic::object())(
      "mapOfStringToStruct", dynamic::object())("setOfEnum", dynamic::array())(
      "setOfI32", dynamic::array())("setOfString", dynamic::array())(
      "unsigned_int64", 123)("mapA", dynamic::object())(
      "mapB", dynamic::object())("cowMap", dynamic::object())(
      "hybridMap", dynamic::object())("hybridList", dynamic::array())(
      "hybridSet", dynamic::array())("hybridUnion", dynamic::object())(
      "hybridStruct",
      dynamic::object("childMap", dynamic::object())("leafI32", 0)(
          "listOfStruct", dynamic::array())("strMap", dynamic::object())(
          "structMap", dynamic::object())("childSet", dynamic::array()))(
      "hybridMapOfI32ToStruct", dynamic::object())(
      "hybridMapOfMap", dynamic::object());
}

TestStruct createTestStruct(folly::dynamic testDyn) {
  return facebook::thrift::from_dynamic<TestStruct>(
      std::move(testDyn), facebook::thrift::dynamic_format::JSON_1);
}

} // namespace

template <bool EnableHybridStorage>
struct TestParams {
  static constexpr auto hybridStorage = EnableHybridStorage;
};

using StorageTestTypes = ::testing::Types<TestParams<false>, TestParams<true>>;

template <typename TestParams>
class RecurseVisitorTests : public ::testing::Test {
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

TYPED_TEST_SUITE(RecurseVisitorTests, StorageTestTypes);

TYPED_TEST(RecurseVisitorTests, TestFullRecurse) {
  auto testDyn = createTestDynamic();
  auto structA = createTestStruct(testDyn);

  auto nodeA = this->initNode(structA);
  std::map<std::vector<std::string>, folly::dynamic> visited;
  auto processPath = [&visited](SimpleTraverseHelper& traverser, auto&& node) {
    folly::dynamic dyn;
    if constexpr (is_cow_type_v<decltype(*node)>) {
      dyn = node->toFollyDynamic();
    } else {
      facebook::thrift::to_dynamic(
          dyn, *node, facebook::thrift::dynamic_format::JSON_1);
    }
    visited.emplace(traverser.path(), dyn);
  };

  SimpleTraverseHelper traverser;
  RootRecurseVisitor::visit(
      traverser,
      nodeA,
      RecurseVisitOptions(
          RecurseVisitMode::FULL, RecurseVisitOrder::PARENTS_FIRST),
      std::move(processPath));

  std::map<std::vector<std::string>, folly::dynamic> expected = {
      {{}, testDyn},
      {{"cowMap"}, dynamic::object()},
      {{"hybridMap"}, dynamic::object()},
      {{"hybridMapOfI32ToStruct"}, dynamic::object()},
      {{"hybridMapOfMap"}, dynamic::object()},
      {{"hybridList"}, dynamic::array()},
      {{"hybridSet"}, dynamic::array()},
      {{"hybridUnion"}, dynamic::object()},
      {{"hybridStruct"}, testDyn["hybridStruct"]},
      {{"mapOfEnumToStruct"}, testDyn["mapOfEnumToStruct"]},
      {{"inlineBool"}, testDyn["inlineBool"]},
      {{"inlineInt"}, testDyn["inlineInt"]},
      {{"inlineString"}, testDyn["inlineString"]},
      {{"optionalString"}, testDyn["optionalString"]},
      {{"unsigned_int64"}, testDyn["unsigned_int64"]},
      {{"inlineStruct"}, testDyn["inlineStruct"]},
      {{"inlineStruct", "min"}, 10},
      {{"inlineStruct", "max"}, 20},
      {{"inlineStruct", "invert"}, false},
      {{"inlineVariant"}, testDyn["inlineVariant"]},
      {{"inlineVariant", "inlineInt"}, testDyn["inlineVariant"]["inlineInt"]},
      {{"listOfListOfPrimitives"}, dynamic::array()},
      {{"listOfListOfStructs"}, dynamic::array()},
      {{"listOfPrimitives"}, dynamic::array()},
      {{"listOfStructs"}, dynamic::array()},
      {{"mapOfEnumToI32"}, dynamic::object()},
      {{"mapOfI32ToI32"}, dynamic::object()},
      {{"mapOfI32ToListOfStructs"}, dynamic::object()},
      {{"mapOfI32ToSetOfString"}, dynamic::object()},
      {{"mapOfI32ToStruct"}, dynamic::object()},
      {{"mapOfStringToI32"}, dynamic::object()},
      {{"mapOfStringToStruct"}, dynamic::object()},
      {{"setOfEnum"}, dynamic::array()},
      {{"setOfI32"}, dynamic::array()},
      {{"setOfString"}, dynamic::array()},
      {{"mapA"}, dynamic::object()},
      {{"mapB"}, dynamic::object()}};

  std::map<std::vector<std::string>, folly::dynamic> hybridNodes = {
      {{"mapOfEnumToStruct", "3"}, testDyn["mapOfEnumToStruct"][3]}};

  std::map<std::vector<std::string>, folly::dynamic> hybridDeepLeaves = {
      {{"hybridStruct", "childMap"}, testDyn["hybridStruct"]["childMap"]},
      {{"hybridStruct", "childSet"}, testDyn["hybridStruct"]["childSet"]},
      {{"hybridStruct", "strMap"}, testDyn["hybridStruct"]["strMap"]},
      {{"hybridStruct", "structMap"}, testDyn["hybridStruct"]["structMap"]},
      {{"hybridStruct", "leafI32"}, testDyn["hybridStruct"]["leafI32"]},
      {{"hybridStruct", "listOfStruct"},
       testDyn["hybridStruct"]["listOfStruct"]},
      {{"mapOfEnumToStruct"}, testDyn["mapOfEnumToStruct"]},
      {{"mapOfEnumToStruct", "3"}, testDyn["mapOfEnumToStruct"][3]},
      {{"mapOfEnumToStruct", "3", "min"},
       testDyn["mapOfEnumToStruct"][3]["min"]},
      {{"mapOfEnumToStruct", "3", "max"},
       testDyn["mapOfEnumToStruct"][3]["max"]},
      {{"mapOfEnumToStruct", "3", "invert"},
       testDyn["mapOfEnumToStruct"][3]["invert"]}};

  if (this->isHybridStorage()) {
    for (const auto& entry : hybridNodes) {
      expected.insert(entry);
    }
  } else {
    for (const auto& entry : hybridDeepLeaves) {
      expected.insert(entry);
    }
  }

  EXPECT_EQ(visited.size(), expected.size());
  for (auto& [path, dyn] : visited) {
    EXPECT_EQ(dyn, visited[path])
        << "Path /" << folly::join('/', path) << " does not match visited";
  }
  for (auto& [path, dyn] : expected) {
    EXPECT_EQ(dyn, visited[path])
        << "Path /" << folly::join('/', path) << " does not match expected";
  }
}

TYPED_TEST(RecurseVisitorTests, TestLeafRecurse) {
  auto testDyn = createTestDynamic();
  auto structA = createTestStruct(testDyn);

  auto nodeA = this->initNode(structA);
  std::map<std::vector<std::string>, folly::dynamic> visited;
  auto processPath = [&visited](SimpleTraverseHelper& traverser, auto&& node) {
    folly::dynamic dyn;
    if constexpr (is_cow_type_v<decltype(*node)>) {
      dyn = node->toFollyDynamic();
    } else {
      facebook::thrift::to_dynamic(
          dyn, *node, facebook::thrift::dynamic_format::JSON_1);
    }
    visited.emplace(traverser.path(), dyn);
  };

  SimpleTraverseHelper traverser;
  RootRecurseVisitor::visit(
      traverser,
      nodeA,
      RecurseVisitOptions(
          RecurseVisitMode::LEAVES, RecurseVisitOrder::PARENTS_FIRST),
      processPath);

  std::map<std::vector<std::string>, folly::dynamic> expected = {
      {{"inlineBool"}, testDyn["inlineBool"]},
      {{"inlineInt"}, testDyn["inlineInt"]},
      {{"inlineString"}, testDyn["inlineString"]},
      {{"optionalString"}, testDyn["optionalString"]},
      {{"unsigned_int64"}, testDyn["unsigned_int64"]},
      {{"inlineStruct", "min"}, 10},
      {{"inlineStruct", "max"}, 20},
      {{"inlineStruct", "invert"}, false},
      {{"inlineVariant", "inlineInt"}, testDyn["inlineVariant"]["inlineInt"]}};

  std::map<std::vector<std::string>, folly::dynamic> hybridDeepLeaves = {
      {{"mapOfEnumToStruct", "3", "min"},
       testDyn["mapOfEnumToStruct"][3]["min"]},
      {{"mapOfEnumToStruct", "3", "max"},
       testDyn["mapOfEnumToStruct"][3]["max"]},
      {{"mapOfEnumToStruct", "3", "invert"},
       testDyn["mapOfEnumToStruct"][3]["invert"]},
      {{"hybridStruct", "leafI32"}, testDyn["hybridStruct"]["leafI32"]}};

  std::map<std::vector<std::string>, folly::dynamic> hybridNodes = {
      {{"mapOfEnumToStruct", "3"}, testDyn["mapOfEnumToStruct"][3]},
      {{"hybridSet"}, testDyn["hybridSet"]},
      {{"hybridUnion"}, testDyn["hybridUnion"]},
      {{"hybridStruct"}, testDyn["hybridStruct"]}};

  if (this->isHybridStorage()) {
    for (const auto& entry : hybridNodes) {
      expected.insert(entry);
    }
  } else {
    for (const auto& entry : hybridDeepLeaves) {
      expected.insert(entry);
    }
  }

  EXPECT_EQ(visited.size(), expected.size());
  for (auto& [path, dyn] : visited) {
    EXPECT_EQ(dyn, expected[path])
        << "Path /" << folly::join('/', path) << " does not match visited";
  }
  for (auto& [path, dyn] : expected) {
    EXPECT_EQ(dyn, visited[path])
        << "Path /" << folly::join('/', path) << " does not match expected";
  }

  visited.clear();
  RootRecurseVisitor::visit(
      traverser,
      nodeA,
      RecurseVisitOptions(
          RecurseVisitMode::LEAVES, RecurseVisitOrder::PARENTS_FIRST, true),
      processPath);

  expected = {
      {{"1"}, testDyn["inlineBool"]},
      {{"2"}, testDyn["inlineInt"]},
      {{"3"}, testDyn["inlineString"]},
      {{"22"}, testDyn["optionalString"]},
      {{"23"}, testDyn["unsigned_int64"]},
      {{"4", "1"}, 10},
      {{"4", "2"}, 20},
      {{"4", "3"}, false},
      {{"21", "2"}, testDyn["inlineVariant"]["inlineInt"]}};

  hybridDeepLeaves = {
      {{"15", "3", "1"}, testDyn["mapOfEnumToStruct"][3]["min"]},
      {{"15", "3", "2"}, testDyn["mapOfEnumToStruct"][3]["max"]},
      {{"15", "3", "3"}, testDyn["mapOfEnumToStruct"][3]["invert"]},
      {{"31", "5"}, testDyn["hybridStruct"]["leafI32"]}};

  hybridNodes = {
      {{"15", "3"}, testDyn["mapOfEnumToStruct"][3]},
      {{"29"}, testDyn["hybridSet"]},
      {{"30"}, testDyn["hybridUnion"]},
      {{"31"}, testDyn["hybridStruct"]}};

  if (this->isHybridStorage()) {
    for (const auto& entry : hybridNodes) {
      expected.insert(entry);
    }
  } else {
    for (const auto& entry : hybridDeepLeaves) {
      expected.insert(entry);
    }
  }

  EXPECT_EQ(visited.size(), expected.size());
  for (auto& [path, dyn] : visited) {
    EXPECT_EQ(dyn, expected[path])
        << "Path /" << folly::join('/', path) << " does not match visited";
  }
  for (auto& [path, dyn] : expected) {
    EXPECT_EQ(dyn, visited[path])
        << "Path /" << folly::join('/', path) << " does not match expected";
  }
}

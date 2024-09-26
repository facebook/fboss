// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/nodes/Serializer.h>
#include <fboss/thrift_cow/storage/CowStorage.h>
#include <fboss/thrift_cow/visitors/PatchBuilder.h>
#include <fboss/thrift_cow/visitors/RecurseVisitor.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_fatal_types.h"
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_types.h"

using folly::dynamic;

namespace {

using namespace facebook::fboss::fsdb;
dynamic createTestDynamic() {
  return dynamic::object("tx", true)("rx", false)("name", "testname")(
      "optionalString", "bla")("enumeration", 1)("enumMap", dynamic::object)(
      "member", dynamic::object("min", 10)("max", 20))(
      "variantMember", dynamic::object("integral", 99))(
      "structMap", dynamic::object(3, dynamic::object("min", 100)("max", 200)))(
      "structList", dynamic::array())("enumSet", dynamic::array())(
      "integralSet", dynamic::array())("mapOfStringToI32", dynamic::object())(
      "listOfPrimitives", dynamic::array())("setOfI32", dynamic::array())(
      "stringToStruct", dynamic::object())("listTypedef", dynamic::array());
}

TestStruct createTestStruct() {
  auto testDyn = createTestDynamic();
  return facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
}

TestStruct createTestStructForExtendedTests() {
  auto testDyn = createTestDynamic();
  for (int i = 0; i <= 20; ++i) {
    testDyn["mapOfStringToI32"][fmt::format("test{}", i)] = i;
    testDyn["listOfPrimitives"].push_back(i);
    testDyn["setOfI32"].push_back(i);
  }

  return facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
}

template <typename T, typename = void>
struct IsPublishable : std::false_type {};

template <typename T>
struct IsPublishable<T, std::void_t<decltype(std::declval<T>()->publish())>>
    : std::true_type {};

template <typename Root>
void publishAllNodes(CowStorage<Root>& storage) {
  using namespace facebook::fboss::thrift_cow;
  auto root = storage.root();
  RootRecurseVisitor::visit(
      root,
      RecurseVisitOptions(
          RecurseVisitMode::FULL, RecurseVisitOrder::CHILDREN_FIRST, true),
      [](const std::vector<std::string>& /*path*/, auto&& node) {
        if constexpr (IsPublishable<decltype(node)>::value) {
          node->publish();
        }
      });
  storage.publish();
}

OperDeltaUnit createEmptyDeltaUnit(std::vector<std::string> path) {
  OperDeltaUnit unit;
  unit.path()->raw() = std::move(path);
  return unit;
}

} // namespace

TEST(CowStorageTests, GetThrift) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = CowStorage<TestStruct>(testStruct);

  EXPECT_EQ(storage.get(root.tx()).value(), true);
  EXPECT_EQ(storage.get(root.rx()).value(), false);
  EXPECT_EQ(storage.get(root.member()).value(), testStruct.member().value());
  EXPECT_EQ(
      storage.get(root.structMap()[3]).value(), testStruct.structMap()->at(3));
  EXPECT_EQ(storage.get(root).value(), testStruct);
}

TEST(CowStorageTests, GetEncoded) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = CowStorage<TestStruct>(testStruct);

  auto result = storage.get_encoded(root.tx(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::integral>(
          OperProtocol::SIMPLE_JSON, true));
  result = storage.get_encoded(root.rx(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::integral>(
          OperProtocol::SIMPLE_JSON, false));
  result = storage.get_encoded(root.member(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::structure>(
          OperProtocol::SIMPLE_JSON, *testStruct.member()));
  result = storage.get_encoded(root.structMap()[3], OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::structure>(
          OperProtocol::SIMPLE_JSON, testStruct.structMap()->at(3)));
  result = storage.get_encoded(root, OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::structure>(
          OperProtocol::SIMPLE_JSON, testStruct));
}

TEST(CowStorageTests, GetEncodedMetadata) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = CowStorage<TestStruct>(testStruct);

  auto result = storage.get_encoded(root.tx(), OperProtocol::SIMPLE_JSON);
  EXPECT_FALSE(result.hasError());
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::integral>(
          OperProtocol::SIMPLE_JSON, true));
  result = storage.get_encoded(root, OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::structure>(
          OperProtocol::SIMPLE_JSON, testStruct));

  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  // change tx to false, since we published already, this should clone
  EXPECT_EQ(storage.set(root.tx(), false), std::nullopt);

  result = storage.get_encoded(root.tx(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::integral>(
          OperProtocol::SIMPLE_JSON, false));

  result = storage.get_encoded(root, OperProtocol::SIMPLE_JSON);
  auto testStruct2 = testStruct;
  testStruct2.tx() = false;
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::structure>(
          OperProtocol::SIMPLE_JSON, testStruct2));
}

TEST(CowStorageTests, SetThrift) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = CowStorage<TestStruct>(testStruct);

  EXPECT_EQ(storage.get(root.tx()).value(), true);
  EXPECT_EQ(storage.get(root.rx()).value(), false);
  EXPECT_EQ(storage.get(root.member()).value(), testStruct.member().value());
  EXPECT_EQ(
      storage.get(root.structMap()[3]).value(), testStruct.structMap()->at(3));

  TestStructSimple newMember;
  newMember.min() = 500;
  newMember.max() = 5000;
  TestStructSimple newStructMapMember;
  newStructMapMember.min() = 300;
  newStructMapMember.max() = 3000;

  // change all the fields
  EXPECT_EQ(storage.set(root.tx(), false), std::nullopt);
  EXPECT_EQ(storage.set(root.rx(), true), std::nullopt);
  EXPECT_EQ(storage.set(root.member(), newMember), std::nullopt);
  EXPECT_EQ(storage.set(root.structMap()[3], newStructMapMember), std::nullopt);

  EXPECT_EQ(storage.get(root.tx()).value(), false);
  EXPECT_EQ(storage.get(root.rx()).value(), true);
  EXPECT_EQ(storage.get(root.member()).value(), newMember);
  EXPECT_EQ(storage.get(root.structMap()[3]).value(), newStructMapMember);
}

TEST(CowStorageTests, AddDynamic) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = CowStorage<TestStruct>(testStruct);

  EXPECT_EQ(storage.get(root.tx()).value(), true);
  EXPECT_EQ(storage.get(root.rx()).value(), false);
  EXPECT_EQ(storage.get(root.member()).value(), testStruct.member().value());
  EXPECT_EQ(
      storage.get(root.structMap()[3]).value(), testStruct.structMap()->at(3));
}

TEST(CowStorageTests, RemoveThrift) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);

  TestStructSimple member1;
  member1.min() = 500;
  member1.max() = 5000;
  TestStructSimple member2;
  member2.min() = 300;
  member2.max() = 3000;

  (*testStruct.structMap())[1] = member1;
  (*testStruct.structMap())[2] = member2;
  (*testStruct.structList()) = {member2, member1, member1};

  auto storage = CowStorage<TestStruct>(testStruct);

  EXPECT_EQ(storage.get(root.tx()).value(), true);
  EXPECT_EQ(storage.get(root.rx()).value(), false);
  EXPECT_EQ(storage.get(root.member()).value(), testStruct.member().value());
  EXPECT_EQ(
      storage.get(root.structMap()[3]).value(), testStruct.structMap()->at(3));

  EXPECT_EQ(storage.get(root.structMap()[1]).value(), member1);
  EXPECT_EQ(storage.get(root.structMap()[2]).value(), member2);
  EXPECT_EQ(storage.get(root.structList()[0]).value(), member2);
  EXPECT_EQ(storage.get(root.structList()[1]).value(), member1);
  EXPECT_EQ(storage.get(root.structList()[2]).value(), member1);

  // delete values
  EXPECT_EQ(storage.remove(root.structMap()[2]), std::nullopt);
  EXPECT_EQ(storage.remove(root.structMap()[3]), std::nullopt);
  EXPECT_EQ(storage.remove(root.structList()[0]), std::nullopt);
  EXPECT_EQ(storage.remove(root.structList()[10]), std::nullopt);

  EXPECT_EQ(storage.get(root.structMap()[1]).value(), member1);
  EXPECT_EQ(
      storage.get(root.structMap()[2]).error(), StorageError::INVALID_PATH);
  EXPECT_EQ(
      storage.get(root.structMap()[3]).error(), StorageError::INVALID_PATH);
  EXPECT_EQ(storage.get(root.structList()[0]).value(), member1);
  EXPECT_EQ(storage.get(root.structList()[1]).value(), member1);
  EXPECT_EQ(
      storage.get(root.structList()[2]).error(), StorageError::INVALID_PATH);
  EXPECT_EQ(
      storage.get(root.structList()[3]).error(), StorageError::INVALID_PATH);
  EXPECT_EQ(
      storage.get(root.structList()[5]).error(), StorageError::INVALID_PATH);
}

TEST(CowStorageTests, PatchDelta) {
  using namespace facebook::fboss::fsdb;
  using namespace apache::thrift::type_class;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = CowStorage<TestStruct>(testStruct);

  // publish to ensure we can patch published storage
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  EXPECT_EQ(storage.get(root.tx()).value(), true);
  EXPECT_EQ(storage.get(root.rx()).value(), false);
  EXPECT_EQ(storage.get(root.optionalString()).value(), "bla");

  auto makeState = [](auto tc, auto val) -> folly::fbstring {
    OperState state;
    using TC = decltype(tc);
    return facebook::fboss::thrift_cow::serialize<TC>(
        OperProtocol::SIMPLE_JSON, val);
  };

  auto deltaUnit = [](std::vector<std::string> path,
                      std::optional<folly::fbstring> oldState,
                      std::optional<folly::fbstring> newState) {
    OperDeltaUnit unit;
    unit.path()->raw() = std::move(path);
    if (oldState) {
      unit.oldState() = *oldState;
    }
    if (newState) {
      unit.newState() = *newState;
    }
    return unit;
  };

  // add values
  OperDelta delta;

  std::vector<OperDeltaUnit> changes = {
      deltaUnit({"tx"}, std::nullopt, makeState(integral{}, false)),
      deltaUnit({"rx"}, std::nullopt, makeState(integral{}, true)),
      deltaUnit({"optionalString"}, makeState(string{}, "bla"), std::nullopt),
      deltaUnit({"member", "min"}, std::nullopt, makeState(integral{}, 100)),
      deltaUnit(
          {"structMap", "5", "min"}, std::nullopt, makeState(integral{}, 1001)),
      deltaUnit(
          {"enumMap", "FIRST", "min"},
          std::nullopt,
          makeState(integral{}, 2001))};
  delta.changes() = std::move(changes);
  delta.protocol() = OperProtocol::SIMPLE_JSON;
  storage.patch(delta);

  EXPECT_EQ(storage.get(root.tx()).value(), false);
  EXPECT_EQ(storage.get(root.rx()).value(), true);
  EXPECT_EQ(
      storage.get(root.optionalString()).error(), StorageError::INVALID_PATH);
  EXPECT_EQ(storage.get(root.member().min()).value(), 100);
  EXPECT_EQ(storage.get(root.structMap()[5].min()).value(), 1001);
  EXPECT_EQ(storage.get(root.enumMap()[TestEnum::FIRST].min()).value(), 2001);
}

TEST(CowStorageTests, EncodedExtendedAccessFieldSimple) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = CowStorage<TestStruct>(testStruct);
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  auto path = ext_path_builder::raw("tx").get();
  auto result = storage.get_encoded_extended(
      path.path()->begin(), path.path()->end(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(result->size(), 1);
  EXPECT_EQ(
      *result->at(0).state()->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::integral>(
          OperProtocol::SIMPLE_JSON, true));
}

TEST(CowStorageTests, EncodedExtendedAccessFieldInContainer) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = CowStorage<TestStruct>(testStruct);
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  auto path = ext_path_builder::raw("structMap").raw("3").get();
  auto result = storage.get_encoded_extended(
      path.path()->begin(), path.path()->end(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(result->size(), 1);
  auto got = facebook::fboss::thrift_cow::
      deserialize<apache::thrift::type_class::structure, TestStructSimple>(
          OperProtocol::SIMPLE_JSON, *result->at(0).state()->contents());
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);
}

TEST(CowStorageTests, EncodedExtendedAccessRegexMap) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = CowStorage<TestStruct>(testStruct);
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  std::map<std::vector<std::string>, int> expected = {
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

  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  auto result = storage.get_encoded_extended(
      path.path()->begin(), path.path()->end(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(expected.size(), result->size());
  for (const auto& taggedState : *result) {
    const auto& elemPath = *taggedState.path()->path();
    const auto& contents = *taggedState.state()->contents();
    auto deserialized = facebook::fboss::thrift_cow::
        deserialize<apache::thrift::type_class::integral, int>(
            OperProtocol::SIMPLE_JSON, contents);
    EXPECT_EQ(expected[elemPath], deserialized)
        << "Mismatch at /" + folly::join('/', elemPath);
  }
}

TEST(CowStorageTests, EncodedExtendedAccessAnyMap) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = CowStorage<TestStruct>(testStruct);
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  std::map<std::vector<std::string>, int> expected = {
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
  auto path = ext_path_builder::raw("mapOfStringToI32").any().get();
  auto result = storage.get_encoded_extended(
      path.path()->begin(), path.path()->end(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(expected.size(), result->size());
  for (const auto& taggedState : *result) {
    const auto& elemPath = *taggedState.path()->path();
    const auto& contents = *taggedState.state()->contents();
    auto deserialized = facebook::fboss::thrift_cow::
        deserialize<apache::thrift::type_class::integral, int>(
            OperProtocol::SIMPLE_JSON, contents);
    EXPECT_EQ(expected[elemPath], deserialized)
        << "Mismatch at /" + folly::join('/', elemPath);
  }
}

TEST(CowStorageTests, EncodedExtendedAccessRegexList) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = CowStorage<TestStruct>(testStruct);
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  std::map<std::vector<std::string>, int> expected = {
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
  auto path = ext_path_builder::raw("listOfPrimitives").regex("1.*").get();
  auto result = storage.get_encoded_extended(
      path.path()->begin(), path.path()->end(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(expected.size(), result->size());
  for (const auto& taggedState : *result) {
    const auto& elemPath = *taggedState.path()->path();
    const auto& contents = *taggedState.state()->contents();
    auto deserialized = facebook::fboss::thrift_cow::
        deserialize<apache::thrift::type_class::integral, int>(
            OperProtocol::SIMPLE_JSON, contents);
    EXPECT_EQ(expected[elemPath], deserialized)
        << "Mismatch at /" + folly::join('/', elemPath);
  }
}

TEST(CowStorageTests, EncodedExtendedAccessAnyList) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = CowStorage<TestStruct>(testStruct);
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  std::map<std::vector<std::string>, int> expected = {
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

  auto path = ext_path_builder::raw("listOfPrimitives").any().get();
  auto result = storage.get_encoded_extended(
      path.path()->begin(), path.path()->end(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(expected.size(), result->size());
  for (const auto& taggedState : *result) {
    const auto& elemPath = *taggedState.path()->path();
    const auto& contents = *taggedState.state()->contents();
    auto deserialized = facebook::fboss::thrift_cow::
        deserialize<apache::thrift::type_class::integral, int>(
            OperProtocol::SIMPLE_JSON, contents);
    EXPECT_EQ(expected[elemPath], deserialized)
        << "Mismatch at /" + folly::join('/', elemPath);
  }
}

TEST(CowStorageTests, EncodedExtendedAccessRegexSet) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = CowStorage<TestStruct>(testStruct);
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  std::map<std::vector<std::string>, int> expected = {
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

  auto path = ext_path_builder::raw("setOfI32").regex("1.*").get();
  auto result = storage.get_encoded_extended(
      path.path()->begin(), path.path()->end(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(expected.size(), result->size());
  for (const auto& taggedState : *result) {
    const auto& elemPath = *taggedState.path()->path();
    const auto& contents = *taggedState.state()->contents();
    auto deserialized = facebook::fboss::thrift_cow::
        deserialize<apache::thrift::type_class::integral, int>(
            OperProtocol::SIMPLE_JSON, contents);
    EXPECT_EQ(expected[elemPath], deserialized)
        << "Mismatch at /" + folly::join('/', elemPath);
  }
}

TEST(CowStorageTests, EncodedExtendedAccessAnySet) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = CowStorage<TestStruct>(testStruct);
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  std::map<std::vector<std::string>, int> expected = {
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

  auto path = ext_path_builder::raw("setOfI32").any().get();
  auto result = storage.get_encoded_extended(
      path.path()->begin(), path.path()->end(), OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(expected.size(), result->size());
  for (const auto& taggedState : *result) {
    const auto& elemPath = *taggedState.path()->path();
    const auto& contents = *taggedState.state()->contents();
    auto deserialized = facebook::fboss::thrift_cow::
        deserialize<apache::thrift::type_class::integral, int>(
            OperProtocol::SIMPLE_JSON, contents);
    EXPECT_EQ(expected[elemPath], deserialized)
        << "Mismatch at /" + folly::join('/', elemPath);
  }
}

TEST(CowStorageTests, PatchRoot) {
  using namespace facebook::fboss::fsdb;
  using namespace facebook::fboss::thrift_cow;
  auto testStructA = createTestStruct();

  auto storage = CowStorage<TestStruct>(testStructA);
  // In FSDB we only publish root, but just to test PatchApplier functionality,
  // publish all nodes and make sure we modify itermediate nodes properly
  publishAllNodes(storage);

  auto testStructB = testStructA;

  // modify various fields and create a big patch
  testStructB.tx() = false;
  testStructB.name() = "new val";
  testStructB.optionalString().reset();
  testStructB.member()->min() = 432;
  // modify
  (*testStructB.structMap())[3].min() = 77;
  // add
  testStructB.structList()->emplace_back();
  testStructB.structList()[0].min() = 22;
  testStructB.listOfPrimitives()->emplace_back(1);
  testStructB.integralSet()->insert(1);
  testStructB.stringToStruct()["new struct"].min() = 55;

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(testStructA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(testStructB);
  auto patch = PatchBuilder::build(nodeA, nodeB, {});

  storage.patch(std::move(patch));
  EXPECT_EQ(storage.root()->toThrift(), testStructB);

  // reset storage and patch just the one member
  storage = CowStorage<TestStruct>(testStructA);
  publishAllNodes(storage);

  auto memberNodeA = std::make_shared<ThriftStructNode<TestStructSimple>>(
      *testStructA.member());
  auto memberNodeB = std::make_shared<ThriftStructNode<TestStructSimple>>(
      *testStructB.member());

  using TestStructMembers = apache::thrift::reflect_struct<TestStruct>::member;
  patch = PatchBuilder::build(
      memberNodeA,
      memberNodeB,
      {folly::to<std::string>(TestStructMembers::member::id::value)});
  storage.patch(std::move(patch));
  using k = thriftpath_test_tags::strings;
  EXPECT_EQ(
      storage.root()->ref<k::member>()->toThrift(), *testStructB.member());
}

TEST(SubscribableStorageTests, PatchInvalidDeltaPath) {
  using namespace facebook::fboss::fsdb;

  auto testStructA = createTestStruct();
  auto storage = CowStorage<TestStruct>(testStructA);

  OperDelta delta;
  OperDeltaUnit unit;
  unit.path()->raw() = {"invalid", "path"};
  unit.newState() = facebook::fboss::thrift_cow::serialize<
      apache::thrift::type_class::structure>(OperProtocol::BINARY, testStructA);
  delta.changes() = {unit};

  // should fail gracefully
  EXPECT_EQ(storage.patch(delta), StorageError::INVALID_PATH);

  // partially valid path should still fail
  unit.path()->raw() = {"inlineStruct", "invalid", "path"};
  delta.changes() = {unit};
  EXPECT_EQ(storage.patch(delta), StorageError::INVALID_PATH);
}

TEST(CowStorageTests, PatchEmptyDeltaNonexistentPath) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStructA = createTestStruct();
  auto storage = CowStorage<TestStruct>(testStructA);

  EXPECT_EQ(storage.get(root.mapOfStructs())->size(), 0);
  EXPECT_EQ(storage.get(root.listofStructs())->size(), 0);

  std::vector<OperDeltaUnit> units = {
      // patch invalid map entry
      createEmptyDeltaUnit(root.mapOfStructs()["a"].m()["some"].tokens()),
      createEmptyDeltaUnit(root.mapOfStructs()["b"].l()[1].tokens()),
      createEmptyDeltaUnit(root.mapOfStructs()["b"].s()[1].tokens()),
      createEmptyDeltaUnit(root.mapOfStructs()["c"].u().integral().tokens()),
      createEmptyDeltaUnit(root.mapOfStructs()["d"].o().tokens()),
      // patch invalid list entry
      createEmptyDeltaUnit(root.listofStructs()[0].m()["some"].tokens()),
      createEmptyDeltaUnit(root.listofStructs()[1].l()[1].tokens()),
      createEmptyDeltaUnit(root.listofStructs()[2].s()[1].tokens()),
      createEmptyDeltaUnit(root.listofStructs()[3].u().integral().tokens()),
      createEmptyDeltaUnit(root.listofStructs()[4].o().tokens()),
  };
  for (const auto& unit : units) {
    OperDelta delta;
    delta.changes() = {unit};

    EXPECT_EQ(storage.patch(delta), StorageError::INVALID_PATH);
    // None of the patches should creat the intermediate nodes
    EXPECT_EQ(storage.get(root.mapOfStructs())->size(), 0);
    EXPECT_EQ(storage.get(root.listofStructs())->size(), 0);
  }
}

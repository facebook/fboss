// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/Utility.h>
#include <folly/json/dynamic.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/nodes/Serializer.h>
#include <fboss/thrift_cow/storage/CowStorage.h>
#include <fboss/thrift_cow/visitors/PatchBuilder.h>
#include <fboss/thrift_cow/visitors/RecurseVisitor.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/op/Get.h>
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_types.h"

using folly::dynamic;
using namespace testing;

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

// Populates TestStruct.recursiveMember with a multi-depth self-referential
// (recursive) RecursiveStruct tree, used to exercise self-referential paths:
//   recursiveMember[0]            name="r0"  simpleMember{min=11,  max=111}
//     children[0]                 name="c0"  simpleMember{min=22,  max=222}
//     children[1]                 name="c1"  simpleMember{min=33,  max=333}
//       children[0]               name="gc0" simpleMember{min=44,  max=444}
void populateRecursiveMember(TestStruct& testStruct) {
  auto makeNode = [](std::string name, int32_t min, int32_t max) {
    RecursiveStruct node;
    node.name() = std::move(name);
    node.simpleMember()->min() = min;
    node.simpleMember()->max() = max;
    return node;
  };

  auto r0 = makeNode("r0", 11, 111);
  auto c0 = makeNode("c0", 22, 222);
  auto c1 = makeNode("c1", 33, 333);
  auto gc0 = makeNode("gc0", 44, 444);
  c1.children()->push_back(std::move(gc0));
  r0.children()->push_back(std::move(c0));
  r0.children()->push_back(std::move(c1));
  testStruct.recursiveMember()->push_back(std::move(r0));
}

template <typename T, typename = void>
struct IsPublishable : std::false_type {};

template <typename T>
struct IsPublishable<T, std::void_t<decltype(std::declval<T>()->publish())>>
    : std::true_type {};

template <typename Root, typename Node>
void publishAllNodes(CowStorage<Root, Node>& storage) {
  using namespace facebook::fboss::thrift_cow;
  auto root = storage.root();
  RootRecurseVisitor::visit(
      root,
      RecurseVisitOptions(
          RecurseVisitMode::FULL, RecurseVisitOrder::CHILDREN_FIRST, true),
      [](SimpleTraverseHelper& /*traverser*/, auto&& node) {
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

template <bool EnableHybridStorage>
struct TestParams {
  static constexpr auto hybridStorage = EnableHybridStorage;
};

using StorageTestTypes = ::testing::Types<TestParams<false>, TestParams<true>>;

template <typename TestParams>
class CowStorageTests : public ::testing::Test {
 public:
  auto initStorage(auto val) {
    auto constexpr isHybridStorage = TestParams::hybridStorage;
    using RootType = std::remove_cvref_t<decltype(val)>;
    return CowStorage<
        RootType,
        facebook::fboss::thrift_cow::ThriftStructNode<
            RootType,
            facebook::fboss::thrift_cow::
                ThriftStructResolver<RootType, isHybridStorage>,
            isHybridStorage>>(val);
  }
};

TYPED_TEST_SUITE(CowStorageTests, StorageTestTypes);

TYPED_TEST(CowStorageTests, VerifyOperDeltaForSetOfPrimitives) {
  using namespace facebook::fboss::fsdb;
  using namespace apache::thrift::type_class;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = this->initStorage(testStruct);

  // publish to ensure we can patch published storage
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  auto initial = storage.get(root.setOfStrings()["v1"]).error();
  EXPECT_EQ(initial.code(), StorageError::Code::INVALID_PATH);

  auto makeState = [](auto tc, auto val) -> folly::fbstring {
    OperState state;
    using TC = decltype(tc);
    return facebook::fboss::thrift_cow::serialize<TC>(
        OperProtocol::SIMPLE_JSON, val);
  };

  auto buildOperDelta = [&makeState](std::string val, bool add) -> OperDelta {
    OperDeltaUnit unit;
    std::vector<std::string> path = {"setOfStrings", val};
    unit.path()->raw() = std::move(path);
    if (add) {
      unit.newState() = makeState(string{}, val);
    } else {
      unit.oldState() = makeState(string{}, val);
    }
    std::vector<OperDeltaUnit> changes = {unit};

    OperDelta delta;
    delta.changes() = std::move(changes);
    delta.protocol() = OperProtocol::SIMPLE_JSON;
    return delta;
  };

  auto verifyOperDelta = [&buildOperDelta, &storage, &root](
                             std::string val, bool add) {
    OperDelta delta = buildOperDelta(val, add);

    auto result = storage.patch(delta);
    EXPECT_FALSE(result.has_value());

    auto val1 = storage.get(root.setOfStrings()).value();
    bool contains1 = val1.contains(val);
    EXPECT_EQ(contains1, add);
  };

  // verify patching OperDelta into storage corresponding to
  // adding and removing values from setOfStrings
  verifyOperDelta("v1", true);
  verifyOperDelta("v2", true);
  verifyOperDelta("v1", false);
  verifyOperDelta("v2", false);
}

TYPED_TEST(CowStorageTests, GetThrift) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  populateRecursiveMember(testStruct);

  auto storage = this->initStorage(testStruct);

  EXPECT_EQ(storage.get(root.tx()).value(), true);
  EXPECT_EQ(storage.get(root.rx()).value(), false);
  EXPECT_EQ(storage.get(root.member()).value(), testStruct.member().value());
  EXPECT_EQ(
      storage.get(root.structMap()[3]).value(), testStruct.structMap()->at(3));

  // self-referential (recursive) struct paths via typed thriftpath accessors
  EXPECT_EQ(storage.get(root.recursiveMember()[0].name()).value(), "r0");
  EXPECT_EQ(
      storage.get(root.recursiveMember()[0].simpleMember().min()).value(), 11);
  auto gotSimple =
      storage.get(root.recursiveMember()[0].simpleMember()).value();
  EXPECT_EQ(*gotSimple.min(), 11);
  EXPECT_EQ(*gotSimple.max(), 111);
  // deeper recursion reached via raw token paths (typed accessors stop at the
  // self-referential boundary)
  EXPECT_EQ(
      storage
          .template get<std::string>(
              {"recursiveMember", "0", "children", "1", "name"})
          .value(),
      "c1");
  EXPECT_EQ(
      storage
          .template get<int32_t>(
              {"recursiveMember",
               "0",
               "children",
               "1",
               "children",
               "0",
               "simpleMember",
               "min"})
          .value(),
      44);

  EXPECT_EQ(storage.get(root).value(), testStruct);
}

TYPED_TEST(CowStorageTests, GetRecursiveStructTyped) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  populateRecursiveMember(testStruct);
  auto storage = this->initStorage(testStruct);

  // Typed access to a recursiveMember list element returns a RecursiveStruct.
  auto elem = storage.get(root.recursiveMember()[0]);
  static_assert(
      std::is_same_v<typename decltype(elem)::value_type, RecursiveStruct>);
  ASSERT_TRUE(elem.hasValue());
  EXPECT_EQ(*elem->name(), "r0");
  EXPECT_EQ(*elem->simpleMember()->min(), 11);
  EXPECT_EQ(elem->children()->size(), 2);

  // Typed access to a first-level child (a self-referential element) also
  // returns a RecursiveStruct.
  auto child = storage.get(root.recursiveMember()[0].children()[1]);
  static_assert(
      std::is_same_v<typename decltype(child)::value_type, RecursiveStruct>);
  ASSERT_TRUE(child.hasValue());
  EXPECT_EQ(*child->name(), "c1");
  EXPECT_EQ(*child->simpleMember()->min(), 33);
  EXPECT_EQ(child->children()->size(), 1);
}

TYPED_TEST(CowStorageTests, GetEncoded) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  populateRecursiveMember(testStruct);
  auto storage = this->initStorage(testStruct);

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

  // self-referential (recursive) struct paths
  result = storage.get_encoded(
      root.recursiveMember()[0].simpleMember().min(),
      OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::integral>(OperProtocol::SIMPLE_JSON, 11));
  // deeper recursion reached via raw token paths
  result = storage.get_encoded(
      {"recursiveMember",
       "0",
       "children",
       "1",
       "children",
       "0",
       "simpleMember"},
      OperProtocol::SIMPLE_JSON);
  TestStructSimple deepSimple;
  deepSimple.min() = 44;
  deepSimple.max() = 444;
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::structure>(
          OperProtocol::SIMPLE_JSON, deepSimple));

  result = storage.get_encoded(root, OperProtocol::SIMPLE_JSON);
  EXPECT_EQ(
      *result->contents(),
      facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::structure>(
          OperProtocol::SIMPLE_JSON, testStruct));
}

TYPED_TEST(CowStorageTests, GetEncodedMetadata) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = this->initStorage(testStruct);

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

TYPED_TEST(CowStorageTests, SetThrift) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  populateRecursiveMember(testStruct);
  auto storage = this->initStorage(testStruct);

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

  // self-referential (recursive) struct paths: set a leaf via typed accessor
  EXPECT_EQ(
      storage.get(root.recursiveMember()[0].simpleMember().min()).value(), 11);
  EXPECT_EQ(
      storage.set(root.recursiveMember()[0].simpleMember().min(), 999),
      std::nullopt);
  EXPECT_EQ(
      storage.get(root.recursiveMember()[0].simpleMember().min()).value(), 999);

  // set a leaf at a deeper recursion level via raw token path
  EXPECT_EQ(
      storage.template set<int32_t>(
          {"recursiveMember",
           "0",
           "children",
           "1",
           "children",
           "0",
           "simpleMember",
           "min"},
          888),
      std::nullopt);
  EXPECT_EQ(
      storage
          .template get<int32_t>(
              {"recursiveMember",
               "0",
               "children",
               "1",
               "children",
               "0",
               "simpleMember",
               "min"})
          .value(),
      888);

  // replace an entire struct member reached through a recursive path
  TestStructSimple newRecursiveSimple;
  newRecursiveSimple.min() = 1;
  newRecursiveSimple.max() = 2;
  EXPECT_EQ(
      storage.set(
          {"recursiveMember", "0", "children", "0", "simpleMember"},
          newRecursiveSimple),
      std::nullopt);
  EXPECT_EQ(
      storage
          .template get<TestStructSimple>(
              {"recursiveMember", "0", "children", "0", "simpleMember"})
          .value(),
      newRecursiveSimple);
}

TYPED_TEST(CowStorageTests, AddDynamic) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = this->initStorage(testStruct);

  EXPECT_EQ(storage.get(root.tx()).value(), true);
  EXPECT_EQ(storage.get(root.rx()).value(), false);
  EXPECT_EQ(storage.get(root.member()).value(), testStruct.member().value());
  EXPECT_EQ(
      storage.get(root.structMap()[3]).value(), testStruct.structMap()->at(3));
}

TYPED_TEST(CowStorageTests, RemoveThrift) {
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
  populateRecursiveMember(testStruct);

  auto storage = this->initStorage(testStruct);

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
      storage.get(root.structMap()[2]).error().code(),
      StorageError::Code::INVALID_PATH);
  EXPECT_EQ(
      storage.get(root.structMap()[3]).error().code(),
      StorageError::Code::INVALID_PATH);
  EXPECT_EQ(storage.get(root.structList()[0]).value(), member1);
  EXPECT_EQ(storage.get(root.structList()[1]).value(), member1);
  EXPECT_EQ(
      storage.get(root.structList()[2]).error().code(),
      StorageError::Code::INVALID_PATH);
  EXPECT_EQ(
      storage.get(root.structList()[3]).error().code(),
      StorageError::Code::INVALID_PATH);
  EXPECT_EQ(
      storage.get(root.structList()[5]).error().code(),
      StorageError::Code::INVALID_PATH);

  // self-referential (recursive) struct paths
  EXPECT_EQ(
      storage
          .template get<std::string>(
              {"recursiveMember",
               "0",
               "children",
               "1",
               "children",
               "0",
               "name"})
          .value(),
      "gc0");
  // remove a deeply-nested recursive element
  EXPECT_EQ(
      storage.remove(
          std::vector<std::string>{
              "recursiveMember", "0", "children", "1", "children", "0"}),
      std::nullopt);
  EXPECT_EQ(
      storage
          .template get<std::string>(
              {"recursiveMember",
               "0",
               "children",
               "1",
               "children",
               "0",
               "name"})
          .error()
          .code(),
      StorageError::Code::INVALID_PATH);
  // the parent recursive node is left intact
  EXPECT_EQ(
      storage
          .template get<std::string>(
              {"recursiveMember", "0", "children", "1", "name"})
          .value(),
      "c1");
  // removing a list element shifts subsequent recursive children down
  EXPECT_EQ(
      storage.remove(
          std::vector<std::string>{"recursiveMember", "0", "children", "0"}),
      std::nullopt);
  EXPECT_EQ(
      storage
          .template get<std::string>(
              {"recursiveMember", "0", "children", "0", "name"})
          .value(),
      "c1");
  EXPECT_EQ(
      storage
          .template get<std::string>(
              {"recursiveMember", "0", "children", "1", "name"})
          .error()
          .code(),
      StorageError::Code::INVALID_PATH);
  // typed removal of the whole recursive list element
  EXPECT_EQ(storage.remove(root.recursiveMember()[0]), std::nullopt);
  EXPECT_EQ(storage.get(root.recursiveMember())->size(), 0);
}

TYPED_TEST(CowStorageTests, PatchDelta) {
  using namespace facebook::fboss::fsdb;
  using namespace apache::thrift::type_class;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  populateRecursiveMember(testStruct);
  auto storage = this->initStorage(testStruct);

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
          makeState(integral{}, 2001)),
      // self-referential (recursive) struct paths
      deltaUnit(
          {"recursiveMember", "0", "simpleMember", "min"},
          std::nullopt,
          makeState(integral{}, 777)),
      // deeper recursion level
      deltaUnit(
          {"recursiveMember",
           "0",
           "children",
           "1",
           "children",
           "0",
           "simpleMember",
           "min"},
          std::nullopt,
          makeState(integral{}, 555))};
  delta.changes() = std::move(changes);
  delta.protocol() = OperProtocol::SIMPLE_JSON;
  storage.patch(delta);

  EXPECT_EQ(storage.get(root.tx()).value(), false);
  EXPECT_EQ(storage.get(root.rx()).value(), true);
  EXPECT_EQ(
      storage.get(root.optionalString()).error().code(),
      StorageError::Code::INVALID_PATH);
  EXPECT_EQ(storage.get(root.member().min()).value(), 100);
  EXPECT_EQ(storage.get(root.structMap()[5].min()).value(), 1001);
  EXPECT_EQ(storage.get(root.enumMap()[TestEnum::FIRST].min()).value(), 2001);
  EXPECT_EQ(
      storage.get(root.recursiveMember()[0].simpleMember().min()).value(), 777);
  EXPECT_EQ(
      storage
          .template get<int32_t>(
              {"recursiveMember",
               "0",
               "children",
               "1",
               "children",
               "0",
               "simpleMember",
               "min"})
          .value(),
      555);
}

template <typename Node>
OperDelta buildOperDelta(
    const std::shared_ptr<Node>& oldNode,
    const std::shared_ptr<Node>& newNode,
    const std::vector<std::string>& basePath = {},
    bool outputIdPaths = false) {
  std::vector<OperDeltaUnit> operDeltaUnits{};

  bool operDeltaWithOldState = true;

  auto makeOperDeltaUnit = [](const std::vector<std::string>& path,
                              const auto& oldNode,
                              const auto& newNode,
                              OperProtocol protocol) -> OperDeltaUnit {
    OperDeltaUnit unit;
    unit.path()->raw() = path;
    if (oldNode) {
      unit.oldState() = oldNode->encode(protocol);
    }
    if (newNode) {
      unit.newState() = newNode->encode(protocol);
    }
    return unit;
  };

  auto processDelta =
      [basePath, &operDeltaUnits, &makeOperDeltaUnit, operDeltaWithOldState](
          facebook::fboss::thrift_cow::SimpleTraverseHelper& traverser,
          const auto& oldNode,
          const auto& newNode,
          facebook::fboss::thrift_cow::DeltaElemTag /* visitTag */) {
        std::vector<std::string> fullPath;
        fullPath.reserve(basePath.size() + traverser.path().size());
        fullPath.insert(fullPath.end(), basePath.begin(), basePath.end());
        fullPath.insert(
            fullPath.end(), traverser.path().begin(), traverser.path().end());
        using NodeT = typename std::remove_reference<decltype(oldNode)>::type;
        const NodeT emptyOldNode;
        operDeltaUnits.push_back(makeOperDeltaUnit(
            fullPath,
            (operDeltaWithOldState ? oldNode : emptyOldNode),
            newNode,
            OperProtocol::SIMPLE_JSON));
      };

  facebook::fboss::thrift_cow::SimpleTraverseHelper traverser;
  facebook::fboss::thrift_cow::RootDeltaVisitor::visit(
      oldNode,
      newNode,
      facebook::fboss::thrift_cow::DeltaVisitOptions(
          facebook::fboss::thrift_cow::DeltaVisitMode::MINIMAL,
          facebook::fboss::thrift_cow::DeltaVisitOrder::PARENTS_FIRST,
          outputIdPaths),
      std::move(processDelta));

  OperDelta delta;
  delta.changes() = std::move(operDeltaUnits);
  const OperProtocol protocol = OperProtocol::SIMPLE_JSON;
  delta.protocol() = protocol;

  return delta;
}

TYPED_TEST(CowStorageTests, verifyOperDeltaForOptionalPrimitives) {
  using namespace facebook::fboss::fsdb;
  using namespace apache::thrift::type_class;

  thriftpath::RootThriftPath<TestStruct> root;

  TestStruct testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = this->initStorage(testStruct);

  // publish to ensure we can patch published storage
  storage.publish();
  EXPECT_TRUE(storage.isPublished());

  // start with optional primitives not set
  std::optional<StorageError> patchResult;
  auto queryEnum = storage.get(root.structMap()[3].optionalEnum());
  EXPECT_EQ(queryEnum.error().code(), StorageError::Code::INVALID_PATH);
  auto queryInt = storage.get(root.structMap()[3].optionalIntegral());
  EXPECT_EQ(queryInt.error().code(), StorageError::Code::INVALID_PATH);
  auto queryString = storage.get(root.structMap()[3].optionalString());
  EXPECT_EQ(queryString.error().code(), StorageError::Code::INVALID_PATH);

  // test: add optional primitives
  auto lastVersion = storage;
  auto setResult =
      storage.set(root.structMap()[3].optionalEnum(), TestEnum::SECOND);
  EXPECT_FALSE(setResult.has_value());
  setResult = storage.set(root.structMap()[3].optionalIntegral(), 100);
  EXPECT_FALSE(setResult.has_value());
  setResult = storage.set(root.structMap()[3].optionalString(), "val_1");
  EXPECT_FALSE(setResult.has_value());

  OperDelta delta = buildOperDelta(lastVersion.root(), storage.root());
  patchResult = storage.patch(delta);
  EXPECT_FALSE(patchResult.has_value());
  queryEnum = storage.get(root.structMap()[3].optionalEnum());
  std::optional<TestEnum> valEnum = queryEnum.value();
  EXPECT_TRUE(valEnum.has_value());
  EXPECT_EQ(valEnum.value(), TestEnum::SECOND);
  queryInt = storage.get(root.structMap()[3].optionalIntegral());
  std::optional<int32_t> valInt = queryInt.value();
  EXPECT_TRUE(valInt.has_value());
  EXPECT_EQ(valInt.value(), 100);
  queryString = storage.get(root.structMap()[3].optionalString());
  std::optional<std::string> valString = queryString.value();
  EXPECT_TRUE(valString.has_value());
  EXPECT_EQ(valString.value(), "val_1");

  // test: update optional primitives
  lastVersion = storage;
  setResult = storage.set(root.structMap()[3].optionalEnum(), TestEnum::THIRD);
  EXPECT_FALSE(setResult.has_value());
  setResult = storage.set(root.structMap()[3].optionalIntegral(), 200);
  EXPECT_FALSE(setResult.has_value());
  setResult = storage.set(root.structMap()[3].optionalString(), "val_2");
  EXPECT_FALSE(setResult.has_value());

  delta = buildOperDelta(lastVersion.root(), storage.root());
  patchResult = storage.patch(delta);
  EXPECT_FALSE(patchResult.has_value());
  queryEnum = storage.get(root.structMap()[3].optionalEnum());
  valEnum = queryEnum.value();
  EXPECT_TRUE(valEnum.has_value());
  EXPECT_EQ(valEnum.value(), TestEnum::THIRD);
  queryInt = storage.get(root.structMap()[3].optionalIntegral());
  valInt = queryInt.value();
  EXPECT_TRUE(valInt.has_value());
  EXPECT_EQ(valInt.value(), 200);
  queryString = storage.get(root.structMap()[3].optionalString());
  valString = queryString.value();
  EXPECT_TRUE(valString.has_value());
  EXPECT_EQ(valString.value(), "val_2");

  // test: remove optional primitives
  lastVersion = storage;
  storage.remove(root.structMap()[3].optionalEnum());
  storage.remove(root.structMap()[3].optionalIntegral());
  storage.remove(root.structMap()[3].optionalString());

  delta = buildOperDelta(lastVersion.root(), storage.root());
  patchResult = storage.patch(delta);
  EXPECT_FALSE(patchResult.has_value());
  queryEnum = storage.get(root.structMap()[3].optionalEnum());
  EXPECT_EQ(queryEnum.error().code(), StorageError::Code::INVALID_PATH);
  queryInt = storage.get(root.structMap()[3].optionalIntegral());
  EXPECT_EQ(queryInt.error().code(), StorageError::Code::INVALID_PATH);
  queryString = storage.get(root.structMap()[3].optionalString());
  EXPECT_EQ(queryString.error().code(), StorageError::Code::INVALID_PATH);
}

TYPED_TEST(CowStorageTests, EncodedExtendedAccessFieldSimple) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = this->initStorage(testStruct);
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

TYPED_TEST(CowStorageTests, EncodedExtendedAccessFieldInContainer) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = this->initStorage(testStruct);
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

TYPED_TEST(CowStorageTests, EncodedExtendedAccessRegexMap) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = this->initStorage(testStruct);
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

TYPED_TEST(CowStorageTests, EncodedExtendedAccessAnyMap) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = this->initStorage(testStruct);
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

TYPED_TEST(CowStorageTests, EncodedExtendedAccessRegexList) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = this->initStorage(testStruct);
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

TYPED_TEST(CowStorageTests, EncodedExtendedAccessAnyList) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = this->initStorage(testStruct);
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

TYPED_TEST(CowStorageTests, EncodedExtendedAccessRegexSet) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = this->initStorage(testStruct);
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

TYPED_TEST(CowStorageTests, EncodedExtendedAccessAnySet) {
  auto testStruct = createTestStructForExtendedTests();

  auto storage = this->initStorage(testStruct);
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

TYPED_TEST(CowStorageTests, PatchRoot) {
  using namespace facebook::fboss::fsdb;
  using namespace facebook::fboss::thrift_cow;
  auto testStructA = createTestStruct();

  auto storage = this->initStorage(testStructA);
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
  storage = this->initStorage(testStructA);
  publishAllNodes(storage);

  auto memberNodeA = std::make_shared<ThriftStructNode<TestStructSimple>>(
      *testStructA.member());
  auto memberNodeB = std::make_shared<ThriftStructNode<TestStructSimple>>(
      *testStructB.member());

  patch = PatchBuilder::build(
      memberNodeA,
      memberNodeB,
      {folly::to<std::string>(folly::to_underlying(
          apache::thrift::op::
              get_field_id_v<TestStruct, apache::thrift::ident::member>))});
  storage.patch(std::move(patch));
  namespace k = apache::thrift::ident;
  EXPECT_EQ(
      storage.root()->template ref<k::member>()->toThrift(),
      *testStructB.member());
}

TYPED_TEST(CowStorageTests, PatchInvalidDeltaPath) {
  using namespace facebook::fboss::fsdb;

  auto testStructA = createTestStruct();
  auto storage = this->initStorage(testStructA);

  OperDelta delta;
  OperDeltaUnit unit;
  unit.path()->raw() = {"invalid", "path"};
  unit.newState() = facebook::fboss::thrift_cow::serialize<
      apache::thrift::type_class::structure>(OperProtocol::BINARY, testStructA);
  delta.changes() = {unit};

  // should fail gracefully
  EXPECT_EQ(storage.patch(delta)->code(), StorageError::Code::INVALID_PATH);

  // partially valid path should still fail
  unit.path()->raw() = {"inlineStruct", "invalid", "path"};
  delta.changes() = {unit};
  EXPECT_EQ(storage.patch(delta)->code(), StorageError::Code::INVALID_PATH);
}

TYPED_TEST(CowStorageTests, PatchEmptyDeltaNonexistentPath) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStructA = createTestStruct();
  auto storage = this->initStorage(testStructA);

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

    EXPECT_EQ(storage.patch(delta)->code(), StorageError::Code::INVALID_PATH);
    // None of the patches should creat the intermediate nodes
    EXPECT_EQ(storage.get(root.mapOfStructs())->size(), 0);
    EXPECT_EQ(storage.get(root.listofStructs())->size(), 0);
  }
}

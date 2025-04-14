// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/nodes/Serializer.h>
#include <fboss/thrift_cow/storage/CowStorage.h>
#include <fboss/thrift_cow/visitors/PatchBuilder.h>
#include <fboss/thrift_cow/visitors/RecurseVisitor.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/hybrid_storage_test.h" // @manual=//fboss/fsdb/tests:hybrid_storage_test-cpp2-thriftpath
#include "fboss/fsdb/tests/gen-cpp2/hybrid_storage_test_types.h"

using folly::dynamic;
using namespace testing;

namespace {

using namespace facebook::fboss::fsdb;
dynamic createTestDynamic() {
  return dynamic::object("tx", true)
      // ("rx", false)("name", "testname")(
      // "optionalString", "bla")("enumeration", 1)("enumMap", dynamic::object)(
      // "member", dynamic::object("min", 10)("max", 20))(
      // "variantMember", dynamic::object("integral", 99))(
      // "structMap", dynamic::object(3, dynamic::object("min", 100)("max",
      // 200)))( "structList", dynamic::array())("enumSet", dynamic::array())(
      // "integralSet", dynamic::array())
      ("mapOfStringToI32", dynamic::object())
      // ("listOfPrimitives", dynamic::array())("setOfI32", dynamic::array())(
      // "stringToStruct", dynamic::object())("listTypedef", dynamic::array())
      ;
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

TYPED_TEST(CowStorageTests, GetThrift) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);

  auto storage = this->initStorage(testStruct);

  auto result = storage.get(root.tx()).value();
  EXPECT_EQ(result, true);
  EXPECT_EQ(storage.get(root.tx()).value(), true);
  EXPECT_EQ(storage.get(root).value(), testStruct);
}

TYPED_TEST(CowStorageTests, GetEncoded) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      createTestDynamic(), facebook::thrift::dynamic_format::JSON_1);
  auto storage = this->initStorage(testStruct);

  auto result = storage.get_encoded(root.tx(), OperProtocol::SIMPLE_JSON);
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
}

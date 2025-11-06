// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/visitors/PathVisitor.h>
#include <fboss/thrift_cow/visitors/tests/VisitorTestUtils.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/thrift_cow/nodes/Types.h"

namespace facebook::fboss::thrift_cow::test {

struct GetVisitedPathsOperator : public BasePathVisitorOperator {
 public:
  GetVisitedPathsOperator()
      : BasePathVisitorOperator(
            ConstSerializableVisitorFunc([this](
                                             const Serializable&,
                                             pv_detail::PathIter begin,
                                             pv_detail::PathIter end) {
              visited.insert(
                  "/" + folly::join('/', std::vector<std::string>(begin, end)));
            })) {}

  const std::set<std::string>& getVisited() {
    return visited;
  }

 private:
  std::set<std::string> visited;
};

template <bool EnableHybridStorage>
struct TestParams {
  static constexpr auto hybridStorage = EnableHybridStorage;
};

using StorageTestTypes = ::testing::Types<TestParams<false>, TestParams<true>>;

template <typename TestParams>
class PathVisitorTests : public ::testing::Test {
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

TYPED_TEST_SUITE(PathVisitorTests, StorageTestTypes);

TYPED_TEST(PathVisitorTests, AccessField) {
  auto structA = createSimpleTestStruct();

  auto nodeA = this->initNode(structA);
  folly::dynamic dyn;
  auto processPath = pvlambda([&dyn](const auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    dyn = node.toFollyDynamic();
  });
  std::vector<std::string> path{"inlineInt"};
  auto result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_EQ(dyn.asInt(), 54);

  // inlineInt
  path = {"2"};
  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_EQ(dyn.asInt(), 54);

  // inlineVariant/inlineInt
  path = {"21", "2"};
  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_EQ(dyn.asInt(), 99);

  // cowMap
  path = {"cowMap", "1"};
  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_TRUE(dyn.asBool());

  // mapOfI32ToStruct/20/min
  path = {"mapOfI32ToStruct", "20", "min"};
  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_EQ(400, dyn.asInt());

  // mapOfI32ToListOfStructs
  path = {"mapOfI32ToListOfStructs", "20", "0", "min"};
  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_EQ(100, dyn.asInt());

  // mapOfI32ToSetOfString
  path = {"mapOfI32ToSetOfString", "20", "test1"};
  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_EQ("test1", dyn.asString());

  // mapOfI32ToSetOfString invalid
  path = {"mapOfI32ToSetOfString", "20", "invalid_entry"};
  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.code(), ThriftTraverseResult::Code::NON_EXISTENT_NODE);
}

TYPED_TEST(PathVisitorTests, AccessAtHybridNodeTest) {
  RootTestStruct root;
  ParentTestStruct parent;
  auto testStruct = createSimpleTestStruct();
  parent.mapOfI32ToMapOfStruct() = {{3, {{"4", std::move(testStruct)}}}};
  root.mapOfI32ToMapOfStruct() = {{1, {{"2", std::move(parent)}}}};

  auto nodeA = this->initNode(root);
  folly::dynamic dyn;
  auto processPath = pvlambda([&dyn](Serializable& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    dyn = node.toFollyDynamic();
  });

  // Thrift path terminating at HybridNode - Access
  std::vector<std::string> path{
      "mapOfI32ToMapOfStruct",
      "1",
      "2",
      "mapOfI32ToMapOfStruct",
      "3",
      "4",
      "hybridMapOfI32ToStruct"};
  auto result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_NE(dyn.find(20), dyn.items().end());
  ChildStruct got = facebook::thrift::from_dynamic<ChildStruct>(
      dyn[20], facebook::thrift::dynamic_format::JSON_1);
  EXPECT_EQ(got.leafI32(), 0);
  EXPECT_TRUE(got.optionalUnion().has_value());
  EXPECT_EQ(got.optionalUnion()->inlineString(), "strInUnion");

  // Thrift path terminating at HybridNode - Set
  using TC = apache::thrift::type_class::map<
      apache::thrift::type_class::integral,
      apache::thrift::type_class::structure>;

  std::map<int32_t, ChildStruct> newMap;
  ChildStruct newChild;
  newChild.leafI32() = 100;
  newMap.emplace(30, std::move(newChild));
  folly::fbstring newVal =
      serialize<TC>(fsdb::OperProtocol::SIMPLE_JSON, newMap);
  SetEncodedPathVisitorOperator setOp(fsdb::OperProtocol::SIMPLE_JSON, newVal);

  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      thrift_cow::PathVisitOptions::visitLeaf(true),
      setOp);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");

  // Thrift path terminating at HybridNode - Get
  GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitOptions::visitLeaf(), getOp);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_EQ(getOp.val, newVal);
}

TYPED_TEST(PathVisitorTests, AccessAtHybridThriftContainerTest) {
  RootTestStruct root;
  ParentTestStruct parent;
  auto testStruct = createSimpleTestStruct();
  parent.mapOfI32ToMapOfStruct() = {{3, {{"4", std::move(testStruct)}}}};
  root.mapOfI32ToMapOfStruct() = {{1, {{"2", std::move(parent)}}}};

  auto nodeA = this->initNode(root);
  folly::dynamic dyn;
  auto processPath = pvlambda([&dyn](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    dyn = node.toFollyDynamic();
  });

  // Thrift path at thrift container under HybridNode - Access
  std::vector<std::string> path{
      "mapOfI32ToMapOfStruct",
      "1",
      "2",
      "mapOfI32ToMapOfStruct",
      "3",
      "4",
      "hybridMapOfI32ToStruct",
      "20",
      "structMap"};
  auto result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_NE(dyn.find("30"), dyn.items().end());
  cfg::L4PortRange got = facebook::thrift::from_dynamic<cfg::L4PortRange>(
      dyn["30"], facebook::thrift::dynamic_format::JSON_1);
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);

  // Thrift path at thrift container under HybridNode - Set
  using TC = apache::thrift::type_class::map<
      apache::thrift::type_class::string,
      apache::thrift::type_class::structure>;

  std::map<std::string, cfg::L4PortRange> newMap;
  cfg::L4PortRange newRange;
  newRange.min() = 3000;
  newRange.max() = 4000;
  newMap.emplace("200", std::move(newRange));
  folly::fbstring newVal =
      serialize<TC>(fsdb::OperProtocol::SIMPLE_JSON, newMap);
  SetEncodedPathVisitorOperator setOp(fsdb::OperProtocol::SIMPLE_JSON, newVal);

  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      thrift_cow::PathVisitOptions::visitLeaf(true),
      setOp);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");

  // Thrift path at thrift container under HybridNode  - Get
  GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitOptions::visitLeaf(), getOp);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_EQ(getOp.val, newVal);
}

TYPED_TEST(PathVisitorTests, AccessAtHybridThriftContainerKeyTest) {
  RootTestStruct root;
  ParentTestStruct parent;
  auto testStruct = createSimpleTestStruct();
  parent.mapOfI32ToMapOfStruct() = {{3, {{"4", std::move(testStruct)}}}};
  root.mapOfI32ToMapOfStruct() = {{1, {{"2", std::move(parent)}}}};

  auto nodeA = this->initNode(root);
  folly::dynamic dyn;
  auto processPath = pvlambda([&dyn](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    dyn = node.toFollyDynamic();
  });

  // Thrift path at thrift container key under HybridNode - Access
  std::vector<std::string> path{
      "mapOfI32ToMapOfStruct",
      "1",
      "2",
      "mapOfI32ToMapOfStruct",
      "3",
      "4",
      "hybridMapOfI32ToStruct",
      "20",
      "structMap",
      "30"};
  auto result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  cfg::L4PortRange got = facebook::thrift::from_dynamic<cfg::L4PortRange>(
      dyn, facebook::thrift::dynamic_format::JSON_1);
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);

  // Thrift path at thrift container key under HybridNode - Set
  using TC = apache::thrift::type_class::structure;

  cfg::L4PortRange newRange;
  newRange.min() = 3000;
  newRange.max() = 4000;
  folly::fbstring newVal =
      serialize<TC>(fsdb::OperProtocol::SIMPLE_JSON, newRange);
  SetEncodedPathVisitorOperator setOp(fsdb::OperProtocol::SIMPLE_JSON, newVal);

  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      thrift_cow::PathVisitOptions::visitLeaf(true),
      setOp);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");

  // Thrift path at thrift container key under HybridNode  - Get
  GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitOptions::visitLeaf(), getOp);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_EQ(getOp.val, newVal);
}

TYPED_TEST(PathVisitorTests, AccessFieldInContainer) {
  auto structA = createSimpleTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<
      TestStruct,
      ThriftStructResolver<TestStruct, true>,
      true>>(structA);

  folly::dynamic dyn;
  auto processPath = pvlambda([&dyn](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    dyn = node.toFollyDynamic();
  });
  std::vector<std::string> path{"mapOfEnumToStruct", "3"};
  auto result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  cfg::L4PortRange got = facebook::thrift::from_dynamic<cfg::L4PortRange>(
      dyn, facebook::thrift::dynamic_format::JSON_1);
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);

  path = {"15", "3"};
  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  got = facebook::thrift::from_dynamic<cfg::L4PortRange>(
      dyn, facebook::thrift::dynamic_format::JSON_1);
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);
}

TYPED_TEST(PathVisitorTests, TraversalModeFull) {
  auto structA = createSimpleTestStruct();
  auto nodeA = this->initNode(structA);

  {
    auto op = GetVisitedPathsOperator();
    std::vector<std::string> path{"mapOfEnumToStruct", "3"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitOptions::visitFull(), op);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
    EXPECT_THAT(
        op.getVisited(),
        ::testing::ContainerEq(
            std::set<std::string>{"/", "/3", "/mapOfEnumToStruct/3"}));
  }
  {
    auto op = GetVisitedPathsOperator();
    std::vector<std::string> path{"mapOfI32ToListOfStructs", "20", "0", "min"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitOptions::visitFull(), op);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
    EXPECT_THAT(
        op.getVisited(),
        ::testing::ContainerEq(
            std::set<std::string>{
                "/",
                "/min",
                "/0/min",
                "/20/0/min",
                "/mapOfI32ToListOfStructs/20/0/min"}));
  }
}

TEST(PathVisitorTests, AccessOptional) {
  auto structA = createSimpleTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  std::string got;
  auto processPath = pvlambda([&got](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    got = node.toFollyDynamic().asString();
  });
  std::vector<std::string> path{"optionalString"};
  auto result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  EXPECT_EQ(got, "bla");
  structA.optionalString().reset();
  nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  got.clear();
  result = RootPathVisitor::visit(
      *nodeA,
      path.begin(),
      path.end(),
      PathVisitOptions::visitLeaf(),
      processPath);
  EXPECT_EQ(result.code(), ThriftTraverseResult::Code::NON_EXISTENT_NODE);
  EXPECT_TRUE(got.empty());
}

TYPED_TEST(PathVisitorTests, VisitWithOperators) {
  auto structA = createSimpleTestStruct();
  structA.setOfI32() = {1};

  auto nodeA = this->initNode(structA);

  {
    folly::fbstring newVal = "123";
    SetEncodedPathVisitorOperator setOp(
        fsdb::OperProtocol::SIMPLE_JSON, newVal);
    std::vector<std::string> path{"inlineInt"};

    auto result = RootPathVisitor::visit(
        *nodeA,
        path.begin(),
        path.end(),
        thrift_cow::PathVisitOptions::visitLeaf(true),
        setOp);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");

    GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
    result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitOptions::visitLeaf(), getOp);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
    EXPECT_EQ(getOp.val, "123");
  }

  {
    folly::fbstring newVal = "123";
    SetEncodedPathVisitorOperator setOp(
        fsdb::OperProtocol::SIMPLE_JSON, newVal);
    std::vector<std::string> path{"setOfI32", "1"};
    auto result = RootPathVisitor::visit(
        *nodeA,
        path.begin(),
        path.end(),
        thrift_cow::PathVisitOptions::visitLeaf(true),
        setOp);
    // SetEncodedPathVisitorOperator also handles paths into sets of primitives
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
  }

  {
    folly::fbstring newVal = "123";
    SetEncodedPathVisitorOperator setOp(
        fsdb::OperProtocol::SIMPLE_JSON, newVal);
    std::vector<std::string> path{"mapOfStringToI32", "test1"};

    auto result = RootPathVisitor::visit(
        *nodeA,
        path.begin(),
        path.end(),
        thrift_cow::PathVisitOptions::visitLeaf(true),
        setOp);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");

    GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
    result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitOptions::visitLeaf(), getOp);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
    EXPECT_EQ(getOp.val, "123");
  }

  {
    using TC = apache::thrift::type_class::structure;
    std::vector<std::string> path{"mapOfI32ToStruct", "20"};

    cfg::L4PortRange newRange;
    newRange.min() = 666;
    newRange.max() = 999;
    folly::fbstring newVal =
        serialize<TC>(fsdb::OperProtocol::SIMPLE_JSON, newRange);
    SetEncodedPathVisitorOperator setOp(
        fsdb::OperProtocol::SIMPLE_JSON, newVal);

    auto result = RootPathVisitor::visit(
        *nodeA,
        path.begin(),
        path.end(),
        thrift_cow::PathVisitOptions::visitLeaf(true),
        setOp);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");

    GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
    result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitOptions::visitLeaf(), getOp);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
    EXPECT_EQ(getOp.val, newVal);
    auto encoded = *getOp.val;
    auto buf =
        folly::IOBuf::wrapBufferAsValue(encoded.data(), encoded.length());
    auto getStruct = deserializeBuf<TC, cfg::L4PortRange>(
        fsdb::OperProtocol::SIMPLE_JSON, std::move(buf));
    EXPECT_EQ(getStruct.min(), 666);
    EXPECT_EQ(getStruct.max(), 999);
  }

  {
    using TC = apache::thrift::type_class::structure;
    std::vector<std::string> path{"mapOfI32ToListOfStructs", "20", "0"};

    cfg::L4PortRange newRange;
    newRange.min() = 666;
    newRange.max() = 999;
    folly::fbstring newVal =
        serialize<TC>(fsdb::OperProtocol::SIMPLE_JSON, newRange);
    SetEncodedPathVisitorOperator setOp(
        fsdb::OperProtocol::SIMPLE_JSON, newVal);

    auto result = RootPathVisitor::visit(
        *nodeA,
        path.begin(),
        path.end(),
        thrift_cow::PathVisitOptions::visitLeaf(true),
        setOp);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");

    GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
    result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitOptions::visitLeaf(), getOp);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");
    EXPECT_EQ(getOp.val, newVal);
    auto encoded = *getOp.val;
    auto buf =
        folly::IOBuf::wrapBufferAsValue(encoded.data(), encoded.length());
    auto getStruct = deserializeBuf<TC, cfg::L4PortRange>(
        fsdb::OperProtocol::SIMPLE_JSON, std::move(buf));
    EXPECT_EQ(getStruct.min(), 666);
    EXPECT_EQ(getStruct.max(), 999);
  }

  {
    using TC = apache::thrift::type_class::structure;
    std::vector<std::string> path{"mapOfI32ToSetOfString", "20", "test1"};

    GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitOptions::visitLeaf(), getOp);
    EXPECT_EQ(result.toString(), "ThriftTraverseResult::OK");

    auto encoded = *getOp.val;
    auto buf =
        folly::IOBuf::wrapBufferAsValue(encoded.data(), encoded.length());
    auto getStruct = deserializeBuf<TC, std::string>(
        fsdb::OperProtocol::SIMPLE_JSON, std::move(buf));
    EXPECT_EQ(getStruct, "test1");
  }
}

} // namespace facebook::fboss::thrift_cow::test

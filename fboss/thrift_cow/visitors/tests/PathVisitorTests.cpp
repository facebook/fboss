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
  const std::set<std::string>& getVisited() {
    return visited;
  }

 protected:
  void visit(
      Serializable& /* node */,
      pv_detail::PathIter begin,
      pv_detail::PathIter end) override {
    visited.insert(
        "/" + folly::join('/', std::vector<std::string>(begin, end)));
  }

  template <typename Node>
  void visit(Node& node, pv_detail::PathIter begin, pv_detail::PathIter end)
    requires(!is_cow_type_v<Node>)
  {
    SerializableWrapper wrapper(node);
    visit(wrapper, begin, end);
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
  auto processPath = pvlambda([&dyn](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    if constexpr (std::is_base_of_v<
                      Serializable,
                      std::remove_cvref_t<decltype(node)>>) {
      dyn = node.toFollyDynamic();
    } else {
      facebook::thrift::to_dynamic(
          dyn, node, facebook::thrift::dynamic_format::JSON_1);
    }
  });
  std::vector<std::string> path{"inlineInt"};
  auto result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(dyn.asInt(), 54);

  // inlineInt
  path = {"2"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(dyn.asInt(), 54);

  // inlineVariant/inlineInt
  path = {"21", "2"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(dyn.asInt(), 99);

  // cowMap
  path = {"cowMap", "1"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_TRUE(dyn.asBool());

  // mapOfI32ToStruct/20/min
  path = {"mapOfI32ToStruct", "20", "min"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(400, dyn.asInt());

  // mapOfI32ToListOfStructs
  path = {"mapOfI32ToListOfStructs", "20", "0", "min"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(100, dyn.asInt());

  // mapOfI32ToSetOfString
  path = {"mapOfI32ToSetOfString", "20", "test1"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ("test1", dyn.asString());

  // mapOfI32ToSetOfString invalid
  path = {"mapOfI32ToSetOfString", "20", "invalid_entry"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);
}

TEST(PathVisitorTests, HybridMapPrimitiveAccess) {
  auto structA = createSimpleTestStruct();

  auto nodeA = std::make_shared<ThriftStructNode<
      TestStruct,
      ThriftStructResolver<TestStruct, true>,
      true>>(structA);
  folly::dynamic dyn;
  auto processPath = pvlambda([&dyn](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    if constexpr (std::is_base_of_v<
                      Serializable,
                      std::remove_cvref_t<decltype(node)>>) {
      dyn = node.toFollyDynamic();
    } else {
      facebook::thrift::to_dynamic(
          dyn, node, facebook::thrift::dynamic_format::JSON_1);
    }
  });

  // hybridMap
  {
    std::vector<std::string> path = {"hybridMap"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_TRUE(dyn[1].asBool());
  }
  // hybridMap/1
  {
    std::vector<std::string> path = {"hybridMap", "1"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_TRUE(dyn.asBool());
  }
  // Invalid path
  // hybridMap/2
  {
    std::vector<std::string> path = {"hybridMap", "2"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::INVALID_MAP_KEY);
  }
}
TEST(PathVisitorTests, HybridMapStructAccess) {
  auto structA = createSimpleTestStruct();

  auto nodeA = std::make_shared<ThriftStructNode<
      TestStruct,
      ThriftStructResolver<TestStruct, true>,
      true>>(structA);
  folly::dynamic dyn;
  auto processPath = pvlambda([&dyn](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    if constexpr (std::is_base_of_v<
                      Serializable,
                      std::remove_cvref_t<decltype(node)>>) {
      dyn = node.toFollyDynamic();
    } else {
      facebook::thrift::to_dynamic(
          dyn, node, facebook::thrift::dynamic_format::JSON_1);
    }
  });
  // hybridMapOfI32ToStruct
  {
    std::vector<std::string> path = {"hybridMapOfI32ToStruct"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_NE(dyn.find(20), dyn.items().end());
    cfg::L4PortRange got;

    got = facebook::thrift::from_dynamic<cfg::L4PortRange>(
        dyn[20], facebook::thrift::dynamic_format::JSON_1);
    EXPECT_EQ(*got.min(), 400);
    EXPECT_EQ(*got.max(), 600);
  }
  // hybridMapOfI32ToStruct/20
  {
    std::vector<std::string> path = {"hybridMapOfI32ToStruct", "20"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    cfg::L4PortRange got;

    got = facebook::thrift::from_dynamic<cfg::L4PortRange>(
        dyn, facebook::thrift::dynamic_format::JSON_1);
    EXPECT_EQ(*got.min(), 400);
    EXPECT_EQ(*got.max(), 600);
  }
  // Invalid path
  // hybridMapOfI32ToStruct/30
  {
    std::vector<std::string> path = {"hybridMapOfI32ToStruct", "30"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::INVALID_MAP_KEY);
  }
  // hybridMapOfI32ToStruct/20/min
  {
    std::vector<std::string> path = {"hybridMapOfI32ToStruct", "20", "min"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    XLOG(INFO) << folly::toJson(dyn);
    EXPECT_EQ(dyn.asInt(), 400);
  }

  // hybridMapOfI32ToStruct/20/max
  {
    std::vector<std::string> path = {"hybridMapOfI32ToStruct", "20", "max"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_EQ(dyn.asInt(), 600);
  }

  // invalid struct member
  // hybridMapOfI32ToStruct/20/foo
  {
    std::vector<std::string> path = {"hybridMapOfI32ToStruct", "20", "foo"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::INVALID_STRUCT_MEMBER);
  }
  // FULL visit mode
  {
    auto op = GetVisitedPathsOperator();
    std::vector<std::string> path{"hybridMapOfI32ToStruct", "20", "max"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::FULL, op);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_THAT(
        op.getVisited(),
        ::testing::ContainerEq(std::set<std::string>{
            "/", "/20/max", "/max", "/hybridMapOfI32ToStruct/20/max"}));
  }
}

TEST(PathVisitorTests, HybridMapOfMapAccess) {
  auto structA = createSimpleTestStruct();

  auto nodeA = std::make_shared<ThriftStructNode<
      TestStruct,
      ThriftStructResolver<TestStruct, true>,
      true>>(structA);
  folly::dynamic dyn;
  auto processPath = pvlambda([&dyn](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    if constexpr (std::is_base_of_v<
                      Serializable,
                      std::remove_cvref_t<decltype(node)>>) {
      dyn = node.toFollyDynamic();
    } else {
      facebook::thrift::to_dynamic(
          dyn, node, facebook::thrift::dynamic_format::JSON_1);
    }
  });
  // hybridMapOfMap
  {
    std::vector<std::string> path = {"hybridMapOfMap"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_NE(dyn.find(10), dyn.items().end());
    EXPECT_NE(dyn[10].find(20), dyn[10].items().end());
    EXPECT_EQ(dyn[10][20].asInt(), 30);
  }

  // hybridMapOfMap/10
  {
    std::vector<std::string> path = {"hybridMapOfMap", "10"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_NE(dyn.find(20), dyn.items().end());
    EXPECT_EQ(dyn[20].asInt(), 30);
  }

  // hybridMapOfMap/10/20
  {
    std::vector<std::string> path = {"hybridMapOfMap", "10", "20"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_EQ(dyn.asInt(), 30);
  }

  // Invalid path
  // hybridMapOfMap/10/30
  {
    std::vector<std::string> path = {"hybridMapOfMap", "10", "30"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::INVALID_MAP_KEY);
  }
  // full visit mode
  {
    auto op = GetVisitedPathsOperator();
    std::vector<std::string> path{"hybridMapOfMap", "10", "20"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::FULL, op);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_THAT(
        op.getVisited(),
        ::testing::ContainerEq(std::set<std::string>{
            "/", "/10/20", "/20", "/hybridMapOfMap/10/20"}));
  }
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
    if constexpr (std::is_base_of_v<
                      Serializable,
                      std::remove_cvref_t<decltype(node)>>) {
      dyn = node.toFollyDynamic();
    } else {
      facebook::thrift::to_dynamic(
          dyn, node, facebook::thrift::dynamic_format::JSON_1);
    }
  });
  std::vector<std::string> path{"mapOfEnumToStruct", "3"};
  auto result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  cfg::L4PortRange got = facebook::thrift::from_dynamic<cfg::L4PortRange>(
      dyn, facebook::thrift::dynamic_format::JSON_1);
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);

  path = {"15", "3"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
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
        *nodeA, path.begin(), path.end(), PathVisitMode::FULL, op);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_THAT(
        op.getVisited(),
        ::testing::ContainerEq(
            std::set<std::string>{"/", "/3", "/mapOfEnumToStruct/3"}));
  }
  {
    auto op = GetVisitedPathsOperator();
    std::vector<std::string> path{"mapOfI32ToListOfStructs", "20", "0", "min"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::FULL, op);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_THAT(
        op.getVisited(),
        ::testing::ContainerEq(std::set<std::string>{
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
    if constexpr (std::is_base_of_v<
                      Serializable,
                      std::remove_cvref_t<decltype(node)>>) {
      got = node.toFollyDynamic().asString();
    } else {
      FAIL() << "unexpected non-cow visit";
    }
  });
  std::vector<std::string> path{"optionalString"};
  auto result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(got, "bla");
  structA.optionalString().reset();
  nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  got.clear();
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);
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
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, setOp);
    EXPECT_EQ(result, ThriftTraverseResult::OK);

    GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
    result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, getOp);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_EQ(getOp.val, "123");
  }

  {
    folly::fbstring newVal = "123";
    SetEncodedPathVisitorOperator setOp(
        fsdb::OperProtocol::SIMPLE_JSON, newVal);
    std::vector<std::string> path{"setOfI32", "1"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, setOp);
    // should throw trying to set an immutable node
    EXPECT_EQ(result, ThriftTraverseResult::VISITOR_EXCEPTION);
  }

  {
    folly::fbstring newVal = "123";
    SetEncodedPathVisitorOperator setOp(
        fsdb::OperProtocol::SIMPLE_JSON, newVal);
    std::vector<std::string> path{"mapOfStringToI32", "test1"};

    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, setOp);
    EXPECT_EQ(result, ThriftTraverseResult::OK);

    GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
    result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, getOp);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
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
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, setOp);
    EXPECT_EQ(result, ThriftTraverseResult::OK);

    GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
    result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, getOp);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
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
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, setOp);
    EXPECT_EQ(result, ThriftTraverseResult::OK);

    GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
    result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, getOp);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
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
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, getOp);
    EXPECT_EQ(result, ThriftTraverseResult::OK);

    auto encoded = *getOp.val;
    auto buf =
        folly::IOBuf::wrapBufferAsValue(encoded.data(), encoded.length());
    auto getStruct = deserializeBuf<TC, std::string>(
        fsdb::OperProtocol::SIMPLE_JSON, std::move(buf));
    EXPECT_EQ(getStruct, "test1");
  }
}

} // namespace facebook::fboss::thrift_cow::test

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
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

namespace facebook::fboss::thrift_cow::test {

TEST(PathVisitorTests, AccessField) {
  auto structA = createSimpleTestStruct();

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  folly::dynamic dyn;
  auto processPath = pvlambda([&dyn](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    dyn = node.toFollyDynamic();
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
}

#ifdef __ENABLE_HYBRID_THRIFT_COW_TESTS__
TEST(PathVisitorTests, HybridMapAccess) {
  auto structA = createHybridMapTestStruct();

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  folly::dynamic dyn;
  auto processPath = pvlambda([&dyn](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    dyn = node.toFollyDynamic();
  });

  // hybridMap
  {
    std::vector<std::string> path = {"hybridMap"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::OK);
    EXPECT_TRUE(dyn[1].asBool());
  }
  {
    // TODO: handle traversing beyond hybrid node
    // hybridMap/1
    std::vector<std::string> path = {"hybridMap", "1"};
    auto result = RootPathVisitor::visit(
        *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
    EXPECT_EQ(result, ThriftTraverseResult::VISITOR_EXCEPTION);
  }
}
#endif // __ENABLE_HYBRID_THRIFT_COW_TESTS__

TEST(PathVisitorTests, AccessFieldInContainer) {
  auto structA = createSimpleTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  cfg::L4PortRange got;
  auto processPath = pvlambda([&got](auto& node, auto begin, auto end) {
    EXPECT_EQ(begin, end);
    got = facebook::thrift::from_dynamic<cfg::L4PortRange>(
        node.toFollyDynamic(), facebook::thrift::dynamic_format::JSON_1);
  });

  std::vector<std::string> path{"mapOfEnumToStruct", "3"};
  auto result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);

  path = {"15", "3"};
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, processPath);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(*got.min(), 100);
  EXPECT_EQ(*got.max(), 200);
}

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

 private:
  std::set<std::string> visited;
};

TEST(PathVisitorTests, TraversalModeFull) {
  auto structA = createSimpleTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

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

TEST(PathVisitorTests, VisitWithOperators) {
  auto structA = createSimpleTestStruct();
  structA.setOfI32() = {1};

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);

  std::vector<std::string> path{"inlineInt"};

  folly::fbstring newVal = "123";
  SetEncodedPathVisitorOperator setOp(fsdb::OperProtocol::SIMPLE_JSON, newVal);
  auto result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, setOp);
  EXPECT_EQ(result, ThriftTraverseResult::OK);

  GetEncodedPathVisitorOperator getOp(fsdb::OperProtocol::SIMPLE_JSON);
  result = RootPathVisitor::visit(
      *nodeA, path.begin(), path.end(), PathVisitMode::LEAF, getOp);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(getOp.val, "123");

  std::vector<std::string> path2{"setOfI32", "1"};
  result = RootPathVisitor::visit(
      *nodeA, path2.begin(), path2.end(), PathVisitMode::LEAF, setOp);
  // should throw trying to set an immutable node
  EXPECT_EQ(result, ThriftTraverseResult::VISITOR_EXCEPTION);
}

} // namespace facebook::fboss::thrift_cow::test

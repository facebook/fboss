// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_types.h"
#include "fboss/thrift_visitors/ThriftPathVisitor.h"

using folly::dynamic;

namespace {

using namespace facebook::fboss::fsdb;
TestStruct createTestStruct() {
  dynamic testDyn = dynamic::object("tx", true)("rx", false)(
      "name", "testname")("optionalString", "bla")(
      "member", dynamic::object("min", 10)("max", 20))(
      "variantMember", dynamic::object("integral", 99))(
      "structMap",
      dynamic::object("3", dynamic::object("min", 100)("max", 200)))(
      "enumSet", dynamic::array(1))("integralSet", dynamic::array(5));

  return facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
}

} // namespace

TEST(PathVisitorTests, TraverseOk) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();

  folly::dynamic out;
  auto extractDyn = [&]<class Tag>(auto&& node, Tag) {
    facebook::thrift::to_dynamic<Tag>(
        out, node, facebook::thrift::dynamic_format::JSON_1);
  };

  std::vector<std::string> path = {"tx"};
  auto result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(out, true);

  path = {"rx"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(out, false);

  path = {"name"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(out, "testname");

  path = {"optionalString"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(out, "bla");

  path = {"member"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_TRUE(out.isObject());
  EXPECT_EQ(out["min"], 10);
  EXPECT_EQ(out["max"], 20);

  path = {"member", "min"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(out, 10);

  path = {"variantMember", "integral"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(out, 99);

  path = {"structMap"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_TRUE(out.isObject());
  EXPECT_TRUE(out[3].isObject());
  EXPECT_EQ(out[3]["min"], 100);
  EXPECT_EQ(out[3]["max"], 200);

  path = {"structMap", "3"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_TRUE(out.isObject());
  EXPECT_EQ(out["min"], 100);
  EXPECT_EQ(out["max"], 200);

  path = {"enumSet", "FIRST"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(out, 1);

  path = {"integralSet", "5"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::OK);
  EXPECT_EQ(out, 5);
}

TEST(PathVisitorTests, TraverseNonExistentNode) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();

  // mark it null
  testStruct.optionalString().reset();

  folly::dynamic out;
  auto extractDyn = [&]<class Tag>(auto&& node, Tag) {
    facebook::thrift::to_dynamic<Tag>(
        out, node, facebook::thrift::dynamic_format::JSON_1);
  };

  std::vector<std::string> path = {"optionalString"};
  auto result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);

  path = {"name", "past_leaf"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);

  path = {"enumSet", "SECOND"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);

  path = {"integralSet", "99"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);
}

TEST(PathVisitorTests, TraverseInvalidStructMember) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();

  folly::dynamic out;
  auto extractDyn = [&]<class Tag>(auto&& node, Tag) {
    facebook::thrift::to_dynamic<Tag>(
        out, node, facebook::thrift::dynamic_format::JSON_1);
  };

  std::vector<std::string> path = {"badMember"};
  auto result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::INVALID_STRUCT_MEMBER);
}

TEST(PathVisitorTests, TraverseIncorrectVariantMember) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();

  folly::dynamic out;
  auto extractDyn = [&]<class Tag>(auto&& node, Tag) {
    facebook::thrift::to_dynamic<Tag>(
        out, node, facebook::thrift::dynamic_format::JSON_1);
  };

  std::vector<std::string> path = {"variantMember", "boolean"};
  auto result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::INCORRECT_VARIANT_MEMBER);
}

TEST(PathVisitorTests, TraverseInvalidVariantMember) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();

  folly::dynamic out;
  auto extractDyn = [&]<class Tag>(auto&& node, Tag) {
    facebook::thrift::to_dynamic<Tag>(
        out, node, facebook::thrift::dynamic_format::JSON_1);
  };

  std::vector<std::string> path = {"variantMember", "nonexistent"};
  auto result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::INVALID_VARIANT_MEMBER);
}

TEST(PathVisitorTests, TraverseInvalidArrayIndex) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();

  folly::dynamic out;
  auto extractDyn = [&]<class Tag>(auto&& node, Tag) {
    facebook::thrift::to_dynamic<Tag>(
        out, node, facebook::thrift::dynamic_format::JSON_1);
  };

  std::vector<std::string> path = {"structList", "0"};
  auto result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);

  path = {"structList", "invalid"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::INVALID_ARRAY_INDEX);
}

TEST(PathVisitorTests, TraverseInvalidMapKey) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();

  folly::dynamic out;
  auto extractDyn = [&]<class Tag>(auto&& node, Tag) {
    facebook::thrift::to_dynamic<Tag>(
        out, node, facebook::thrift::dynamic_format::JSON_1);
  };

  std::vector<std::string> path = {"structMap", "0"};
  auto result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::NON_EXISTENT_NODE);

  path = {"structMap", "invalid"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::INVALID_MAP_KEY);
}

TEST(PathVisitorTests, TraverseInvalidSetMember) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();

  folly::dynamic out;
  auto extractDyn = [&]<class Tag>(auto&& node, Tag) {
    facebook::thrift::to_dynamic<Tag>(
        out, node, facebook::thrift::dynamic_format::JSON_1);
  };

  std::vector<std::string> path = {"integralSet", "a string"};
  auto result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::INVALID_SET_MEMBER);

  path = {"enumSet", "blabla"};
  result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), extractDyn);
  EXPECT_EQ(result, ThriftTraverseResult::INVALID_SET_MEMBER);
}

TEST(PathVisitorTests, TraverseVisitorException) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();

  auto willThrow = [&](auto&&, auto) { throw std::runtime_error("boo"); };

  std::vector<std::string> path = {"structMap"};
  auto result = RootThriftPathVisitor<TestStruct>::visit(
      testStruct, path.begin(), path.end(), willThrow);
  EXPECT_EQ(result, ThriftTraverseResult::VISITOR_EXCEPTION);
}

TEST(PathVisitorTests, LazyInstantiate) {
  using namespace facebook::fboss::fsdb;

  using Entry = std::pair<std::vector<std::string>, folly::dynamic>;

  TestStruct testStruct;

  std::vector<Entry> leaves = {
      {{"tx"}, true},
      {{"rx"}, false},
      {{"enumeration"}, 1},
      {{"name"}, "testname"},
      {{"optionalString"}, "bla"},
      {{"member", "min"}, 10},
      {{"member", "max"}, 20},
      {{"variantMember", "integral"}, 99},
      {{"structMap", "3", "min"}, 100},
      {{"structMap", "3", "max"}, 200},
      {{"enumSet", "1"}, 1},
      {{"integralSet", "5"}, 5}};

  using Visitor =
      RootThriftPathVisitorWithOptions<TestStruct, CreateNodeIfMissing>;
  for (auto& [path, val] : leaves) {
    auto setVal = [val = val]<class Tag>(auto&& node, Tag) {
      facebook::thrift::from_dynamic<Tag>(
          node, val, facebook::thrift::dynamic_format::JSON_1);
    };
    auto result =
        Visitor::visit(testStruct, path.begin(), path.end(), std::move(setVal));
    EXPECT_EQ(result, ThriftTraverseResult::OK)
        << "Failed to visit " << folly::join(".", path);
  }

  auto expected = createTestStruct();
  EXPECT_EQ(testStruct, expected)
      << "final structs differ: "
      << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
             testStruct)
      << " does not equal "
      << apache::thrift::SimpleJSONSerializer::serialize<std::string>(expected);
}

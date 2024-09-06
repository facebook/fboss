// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_types.h"
#include "fboss/thrift_visitors/ThriftLeafVisitor.h"

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

TEST(ThriftLeafVisitorTests, TraverseOk) {
  using namespace facebook::fboss::fsdb;

  using Entry = std::pair<std::vector<std::string>, folly::dynamic>;

  auto testStruct = createTestStruct();

  std::vector<Entry> leaves;

  auto addLeaf = [&]<class Tag>(auto&& path, Tag, auto&& node) {
    folly::dynamic dyn;
    facebook::thrift::to_dynamic(
        dyn, node, facebook::thrift::dynamic_format::JSON_1);
    leaves.emplace_back(path, dyn);
  };

  RootThriftLeafVisitor<TestStruct>::visit(testStruct, addLeaf);

  std::vector<Entry> tests = {
      {{"tx"}, true},
      {{"enumeration"}, 1},
      {{"rx"}, false},
      {{"name"}, "testname"},
      {{"optionalString"}, "bla"},
      {{"member", "min"}, 10},
      {{"member", "max"}, 20},
      {{"variantMember", "integral"}, 99},
      {{"structMap", "3", "min"}, 100},
      {{"structMap", "3", "max"}, 200},
      {{"enumSet", "1"}, 1},
      {{"integralSet", "5"}, 5}};

  std::sort(tests.begin(), tests.end());
  std::sort(leaves.begin(), leaves.end());
  EXPECT_EQ(leaves, tests);
}

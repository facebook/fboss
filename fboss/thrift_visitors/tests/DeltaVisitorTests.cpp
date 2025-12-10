// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/thrift_visitors/ThriftDeltaVisitor.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_types.h"

using folly::dynamic;

namespace {

using namespace facebook::fboss::fsdb;
TestStruct createTestStruct() {
  dynamic testDyn = dynamic::object("tx", true)("rx", false)(
      "name", "testname")("optionalString", "bla")(
      "member", dynamic::object("min", 10)("max", 20))(
      "variantMember", dynamic::object("integral", 99))(
      "structMap",
      dynamic::object("3", dynamic::object("min", 100)("max", 200)));

  return facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
}

} // namespace

TEST(DeltaVisitorTests, ChangeOneField) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();
  auto otherStruct = testStruct;
  otherStruct.tx() = false;

  std::set<std::string> differingPaths;
  auto processChange = [&](std::vector<std::string>& path,
                           auto /*tag*/,
                           auto /*oldValue*/,
                           auto /*newValue*/) {
    differingPaths.insert("/" + folly::join('/', path));
  };

  auto result = RootThriftDeltaVisitor<TestStruct>::visit(
      testStruct, otherStruct, processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(std::set<std::string>{"/", "/tx"}));
}

TEST(DeltaVisitorTests, ChangeOneFieldInContainer) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();
  auto otherStruct = testStruct;
  otherStruct.structMap()->at(3).min() = 11;

  std::set<std::string> differingPaths;
  auto processChange = [&](std::vector<std::string>& path,
                           auto /*tag*/,
                           auto /*oldValue*/,
                           auto /*newValue*/) {
    differingPaths.insert("/" + folly::join('/', path));
  };

  auto result = RootThriftDeltaVisitor<TestStruct>::visit(
      testStruct, otherStruct, processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          std::set<std::string>{
              "/", "/structMap", "/structMap/3", "/structMap/3/min"}));
}

TEST(DeltaVisitorTests, SetOptional) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();
  auto otherStruct = testStruct;
  otherStruct.optionalString() = "now I'm set";

  std::set<std::string> differingPaths;
  auto processChange = [&](std::vector<std::string>& path,
                           auto /*tag*/,
                           auto /*oldValue*/,
                           auto /*newValue*/) {
    differingPaths.insert("/" + folly::join('/', path));
  };

  auto result = RootThriftDeltaVisitor<TestStruct>::visit(
      testStruct, otherStruct, processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(std::set<std::string>{"/", "/optionalString"}));
}

TEST(DeltaVisitorTests, AddToMap) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();
  auto otherStruct = testStruct;

  TestStructSimple newOne;
  newOne.min() = 40;
  newOne.max() = 100;
  otherStruct.structMap()->emplace(4, std::move(newOne));

  std::set<std::string> differingPaths;
  auto processChange = [&](std::vector<std::string>& path,
                           auto /*tag*/,
                           auto /*oldValue*/,
                           auto /*newValue*/) {
    differingPaths.insert("/" + folly::join('/', path));
  };

  auto result = RootThriftDeltaVisitor<TestStruct>::visit(
      testStruct, otherStruct, processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          std::set<std::string>{"/", "/structMap", "/structMap/4"}));
}

TEST(DeltaVisitorTests, DeleteFromMap) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();
  auto otherStruct = testStruct;
  otherStruct.structMap()->erase(3);

  std::set<std::string> differingPaths;
  auto processChange = [&](std::vector<std::string>& path,
                           auto /*tag*/,
                           auto /*oldValue*/,
                           auto /*newValue*/) {
    differingPaths.insert("/" + folly::join('/', path));
  };

  auto result = RootThriftDeltaVisitor<TestStruct>::visit(
      testStruct, otherStruct, processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          std::set<std::string>{"/", "/structMap", "/structMap/3"}));
}

TEST(DeltaVisitorTests, AddToList) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();
  auto otherStruct = testStruct;
  TestStructSimple newOne;
  newOne.min() = 40;
  newOne.max() = 100;
  otherStruct.structList()->push_back(std::move(newOne));

  std::set<std::string> differingPaths;
  auto processChange = [&](std::vector<std::string>& path,
                           auto /*tag*/,
                           auto /*oldValue*/,
                           auto /*newValue*/) {
    differingPaths.insert("/" + folly::join('/', path));
  };

  auto result = RootThriftDeltaVisitor<TestStruct>::visit(
      testStruct, otherStruct, processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          std::set<std::string>{"/", "/structList", "/structList/0"}));
}

TEST(DeltaVisitorTests, DeleteFromList) {
  using namespace facebook::fboss::fsdb;

  auto otherStruct = createTestStruct();
  auto testStruct = otherStruct;
  TestStructSimple newOne;
  newOne.min() = 40;
  newOne.max() = 100;
  testStruct.structList()->push_back(std::move(newOne));

  std::set<std::string> differingPaths;
  auto processChange = [&](std::vector<std::string>& path,
                           auto /*tag*/,
                           auto /*oldValue*/,
                           auto /*newValue*/) {
    differingPaths.insert("/" + folly::join('/', path));
  };

  auto result = RootThriftDeltaVisitor<TestStruct>::visit(
      testStruct, otherStruct, processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          std::set<std::string>{"/", "/structList", "/structList/0"}));
}

TEST(DeltaVisitorTests, EditVariantField) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();
  auto otherStruct = testStruct;
  otherStruct.variantMember()->integral() = 1000;

  std::set<std::string> differingPaths;
  auto processChange = [&](std::vector<std::string>& path,
                           auto /*tag*/,
                           auto /*oldValue*/,
                           auto /*newValue*/) {
    differingPaths.insert("/" + folly::join('/', path));
  };

  auto result = RootThriftDeltaVisitor<TestStruct>::visit(
      testStruct, otherStruct, processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          std::set<std::string>{
              "/", "/variantMember", "/variantMember/integral"}));
}

TEST(DeltaVisitorTests, SwitaghVariantField) {
  using namespace facebook::fboss::fsdb;

  auto testStruct = createTestStruct();
  auto otherStruct = testStruct;
  otherStruct.variantMember()->boolean() = true;

  std::set<std::string> differingPaths;
  auto processChange = [&](std::vector<std::string>& path,
                           auto /*tag*/,
                           auto /*oldValue*/,
                           auto /*newValue*/) {
    differingPaths.insert("/" + folly::join('/', path));
  };

  auto result = RootThriftDeltaVisitor<TestStruct>::visit(
      testStruct, otherStruct, processChange);
  EXPECT_EQ(result, true);
  EXPECT_THAT(
      differingPaths,
      ::testing::ContainerEq(
          std::set<std::string>{
              "/",
              "/variantMember",
              "/variantMember/integral",
              "/variantMember/boolean"}));
}

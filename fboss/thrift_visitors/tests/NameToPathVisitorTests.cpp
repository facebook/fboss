// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/Demangle.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

#include <fboss/thrift_visitors/NameToPathVisitor.h>
// @lint-ignore CLANGTIDY
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_types.h"

using namespace facebook::fboss::fsdb;
using namespace thriftpath;
TEST(NameToPathVisitorTests, TraverseOk) {
  RootThriftPath<TestStruct> root;

  std::vector<std::vector<std::string>> paths = {
      {"tx"},
      {"rx"},
      {"name"},
      {"member"},
      {"member", "min"},
      {"member", "max"},
      {"structMap"},
      {"structMap", "0"},
      {"structMap", "0", "min"},
      {"structMap", "0", "max"},
      {"integralSet"},
      {"integralSet", "0"},
      {"integralSet", "1"},
      {"optionalString"},
      {"variantMember"},
      {"variantMember", "integral"},
      {"variantMember", "boolean"},
      {"variantMember", "str"},
      {"structList"},
      {"structList", "0"},
      {"structList", "0", "min"},
      {"structList", "0", "max"},
      {"enumMap"},
      {"enumMap", "1"},
      {"enumMap", "1", "min"},
      {"enumMap", "1", "max"},
      {"enumMap", "FIRST"},
      {"enumMap", "FIRST", "min"},
      {"enumMap", "FIRST", "max"},
      {"enumSet"},
      {"enumSet", "1"},
      {"enumSet", "FIRST"},
      {"enumeration"},
      // using ids
      {"1"}, // tx
      {"2"}, // rx
      {"3"}, // name
      {"4"}, // member
      {"4", "1"}, // member/min
      {"4", "2"}, // member/max
      {"7", "1"}, // variantMember/integral
      {"7", "2"}, // variantMember/boolean
      {"7", "3"}, // variantMember/str
  };

  for (auto& path : paths) {
    auto result = RootNameToPathVisitor<RootThriftPath<TestStruct>>::visit(
        root, path.begin(), path.begin(), path.end(), [&]<class Tag>(Tag) {
          using ThriftTag = typename Tag::Tag;
          using DataType = typename Tag::DataT;

          XLOG(INFO) << " For path : " << folly::join('/', path)
                     << " got thrift tag :"
                     << folly::demangle(typeid(ThriftTag))
                     << " data type: " << folly::demangle(typeid(DataType));
        });
    EXPECT_EQ(result, NameToPathResult::OK)
        << "Failed path: /" + folly::join('/', path);
  }
}

TEST(NameToPathVisitorTests, TraverseNotOk) {
  RootThriftPath<TestStruct> root;

  std::vector<std::vector<std::string>> paths = {
      {"foo"},
      {"/", "foo"},
      {"foo", "bar"},
      // Cannot start at a nested root
      {"max"},
      {"10000"}, // invalid id
      {"7", "1000"}, // variantMember/<invalid id>
  };
  for (auto& path : paths) {
    auto result = RootNameToPathVisitor<RootThriftPath<TestStruct>>::visit(
        root, path.begin(), path.begin(), path.end(), [&](auto /*resolved*/) {
          EXPECT_FALSE(true) << " Should never be called";
        });
    EXPECT_NE(result, NameToPathResult::OK);
  }
}

TEST(NameToPathVisitorTests, AlternateRoots) {
  RootThriftPath<TestStructSimple> testStructSimpleRoot;

  std::vector<std::vector<std::string>> paths = {
      {"min"},
      {"max"},
  };
  for (auto& path : paths) {
    auto result =
        RootNameToPathVisitor<RootThriftPath<TestStructSimple>>::visit(
            testStructSimpleRoot,
            path.begin(),
            path.begin(),
            path.end(),
            [&]<class Tag>(Tag) {
              using ThriftTag = typename Tag::Tag;
              using DataType = typename Tag::DataT;

              XLOG(INFO) << " For path : " << folly::join('/', path)
                         << " got thrift tag :"
                         << folly::demangle(typeid(ThriftTag))
                         << " data type: " << folly::demangle(typeid(DataType));
            });
    EXPECT_EQ(result, NameToPathResult::OK)
        << "Failed path: /" + folly::join('/', path);
  }
}

TEST(NameToPathVisitorTests, RecursiveStruct) {
  RootThriftPath<TestStruct> testStructRoot;

  // Each case pairs an input path with the canonical name/id tokens it must
  // resolve to. Both the name-form and the id-form inputs must resolve to the
  // same tokens, mirroring the NameAndIds test above but exercising a
  // self-referential (recursive) struct reached via TestStruct.recursiveMember
  // (list<RecursiveStruct>) and RecursiveStruct.children
  // (list<RecursiveStruct>).
  struct PathCase {
    std::vector<std::string> input;
    std::vector<std::string> expectedNameTokens;
    std::vector<std::string> expectedIdTokens;
  };

  // recursiveMember=24, simpleMember=2, children=3, name=1; min=1
  const std::vector<PathCase> cases = {
      // recursiveMember/0/simpleMember/min
      {{"recursiveMember", "0", "simpleMember", "min"},
       {"recursiveMember", "0", "simpleMember", "min"},
       {"24", "0", "2", "1"}},
      {{"24", "0", "2", "1"},
       {"recursiveMember", "0", "simpleMember", "min"},
       {"24", "0", "2", "1"}},
      // recursiveMember/0/children/1/name
      // (name has field id 1, so the trailing id token is /1)
      {{"recursiveMember", "0", "children", "1", "name"},
       {"recursiveMember", "0", "children", "1", "name"},
       {"24", "0", "3", "1", "1"}},
      {{"24", "0", "3", "1", "1"},
       {"recursiveMember", "0", "children", "1", "name"},
       {"24", "0", "3", "1", "1"}},
      // recursiveMember/0/children/1/children/0/simpleMember/min
      {{"recursiveMember",
        "0",
        "children",
        "1",
        "children",
        "0",
        "simpleMember",
        "min"},
       {"recursiveMember",
        "0",
        "children",
        "1",
        "children",
        "0",
        "simpleMember",
        "min"},
       {"24", "0", "3", "1", "3", "0", "2", "1"}},
      {{"24", "0", "3", "1", "3", "0", "2", "1"},
       {"recursiveMember",
        "0",
        "children",
        "1",
        "children",
        "0",
        "simpleMember",
        "min"},
       {"24", "0", "3", "1", "3", "0", "2", "1"}},
  };

  for (const auto& testCase : cases) {
    const auto& path = testCase.input;
    auto result = RootNameToPathVisitor<RootThriftPath<TestStruct>>::visit(
        testStructRoot,
        path.begin(),
        path.begin(),
        path.end(),
        [&](const auto& resolved) {
          EXPECT_EQ(resolved.tokens(), testCase.expectedNameTokens);
          EXPECT_EQ(resolved.idTokens(), testCase.expectedIdTokens);
        });
    EXPECT_EQ(result, NameToPathResult::OK)
        << "Failed path: /" + folly::join('/', path);
  }
}

TEST(NameToPathVisitorTests, RecursiveStructInvalidPath) {
  RootThriftPath<TestStruct> testStructRoot;

  // Invalid field access *within* a recursive struct must fail resolution
  // rather than silently succeeding or looping. Each of these dereferences a
  // valid recursiveMember/children chain and then references a non-existent
  // member at the end.
  const std::vector<std::vector<std::string>> invalidPaths = {
      // bogus field name directly under a RecursiveStruct
      {"recursiveMember", "0", "bogusField"},
      // bogus numeric field id under a RecursiveStruct
      {"recursiveMember", "0", "99"},
      // bogus field one level of recursion deep
      {"recursiveMember", "0", "children", "0", "bogusField"},
      // bogus field two levels of recursion deep
      {"recursiveMember", "0", "children", "1", "children", "0", "nope"},
  };

  for (const auto& path : invalidPaths) {
    auto result = RootNameToPathVisitor<RootThriftPath<TestStruct>>::visit(
        testStructRoot,
        path.begin(),
        path.begin(),
        path.end(),
        [&](const auto& /*resolved*/) {
          ADD_FAILURE() << "visitor should not resolve invalid path /"
                        << folly::join('/', path);
        });
    EXPECT_EQ(result, NameToPathResult::INVALID_STRUCT_MEMBER)
        << "Unexpected result for path: /" + folly::join('/', path);
  }
}

TEST(NameToPathVisitorTests, RecursiveStructExtended) {
  RootThriftPath<TestStruct> testStructRoot;

  auto makeRaw = [](const std::string& token) {
    OperPathElem elem;
    elem.set_raw(token);
    return elem;
  };

  // Extended-path (visitExtended) resolution over a recursive struct: a raw
  // extended path descending
  // recursiveMember/0/children/1/children/0/simpleMember/min must resolve to
  // the canonical name and id tokens.
  const std::vector<std::string> nameTokens{
      "recursiveMember",
      "0",
      "children",
      "1",
      "children",
      "0",
      "simpleMember",
      "min"};
  // recursiveMember=24, children=3, simpleMember=2, min=1
  const std::vector<std::string> idTokens{
      "24", "0", "3", "1", "3", "0", "2", "1"};

  std::vector<OperPathElem> extPath;
  extPath.reserve(nameTokens.size());
  for (const auto& token : nameTokens) {
    extPath.push_back(makeRaw(token));
  }

  bool resolved = false;
  auto result =
      RootNameToPathVisitor<RootThriftPath<TestStruct>>::visitExtended(
          testStructRoot,
          extPath.begin(),
          extPath.end(),
          [&](const thriftpath::BasePath& path) {
            resolved = true;
            EXPECT_EQ(path.tokens(), nameTokens);
            EXPECT_EQ(path.idTokens(), idTokens);
          });
  EXPECT_EQ(result, NameToPathResult::OK);
  EXPECT_TRUE(resolved);

  // A bogus field in an extended path over a recursive struct fails too.
  std::vector<OperPathElem> badExtPath{
      makeRaw("recursiveMember"), makeRaw("0"), makeRaw("bogusField")};
  result = RootNameToPathVisitor<RootThriftPath<TestStruct>>::visitExtended(
      testStructRoot,
      badExtPath.begin(),
      badExtPath.end(),
      [&](const thriftpath::BasePath& /*path*/) {
        ADD_FAILURE() << "visitor should not resolve invalid extended path";
      });
  EXPECT_EQ(result, NameToPathResult::INVALID_STRUCT_MEMBER);
}

TEST(NameToPathVisitorTests, RecursiveStructMapKeyed) {
  RootThriftPath<TestStruct> testStructRoot;

  // Same name/id token equivalence as the RecursiveStruct test above, but the
  // recursive type is reached through a map (TestStruct.mapOfRecursiveStruct,
  // map<string, RecursiveStruct>) instead of a list. Map keys are opaque and
  // appear verbatim in both the name and the id tokens.
  struct PathCase {
    std::vector<std::string> input;
    std::vector<std::string> expectedNameTokens;
    std::vector<std::string> expectedIdTokens;
  };

  // mapOfRecursiveStruct=25, name=1, simpleMember=2, children=3; min=1
  const std::vector<PathCase> cases = {
      // mapOfRecursiveStruct/key0/name
      {{"mapOfRecursiveStruct", "key0", "name"},
       {"mapOfRecursiveStruct", "key0", "name"},
       {"25", "key0", "1"}},
      {{"25", "key0", "1"},
       {"mapOfRecursiveStruct", "key0", "name"},
       {"25", "key0", "1"}},
      // mapOfRecursiveStruct/key0/simpleMember/min
      {{"mapOfRecursiveStruct", "key0", "simpleMember", "min"},
       {"mapOfRecursiveStruct", "key0", "simpleMember", "min"},
       {"25", "key0", "2", "1"}},
      // map key followed by a level of self-recursion:
      // mapOfRecursiveStruct/key0/children/1/simpleMember/min
      {{"mapOfRecursiveStruct", "key0", "children", "1", "simpleMember", "min"},
       {"mapOfRecursiveStruct", "key0", "children", "1", "simpleMember", "min"},
       {"25", "key0", "3", "1", "2", "1"}},
      {{"25", "key0", "3", "1", "2", "1"},
       {"mapOfRecursiveStruct", "key0", "children", "1", "simpleMember", "min"},
       {"25", "key0", "3", "1", "2", "1"}},
  };

  for (const auto& testCase : cases) {
    const auto& path = testCase.input;
    auto result = RootNameToPathVisitor<RootThriftPath<TestStruct>>::visit(
        testStructRoot,
        path.begin(),
        path.begin(),
        path.end(),
        [&](const auto& resolved) {
          EXPECT_EQ(resolved.tokens(), testCase.expectedNameTokens);
          EXPECT_EQ(resolved.idTokens(), testCase.expectedIdTokens);
        });
    EXPECT_EQ(result, NameToPathResult::OK)
        << "Failed path: /" + folly::join('/', path);
  }

  // A bogus member under a map-keyed RecursiveStruct must fail resolution.
  const std::vector<std::string> invalidPath{
      "mapOfRecursiveStruct", "key0", "bogusField"};
  auto result = RootNameToPathVisitor<RootThriftPath<TestStruct>>::visit(
      testStructRoot,
      invalidPath.begin(),
      invalidPath.begin(),
      invalidPath.end(),
      [&](const auto& /*resolved*/) {
        ADD_FAILURE() << "visitor should not resolve invalid path /"
                      << folly::join('/', invalidPath);
      });
  EXPECT_EQ(result, NameToPathResult::INVALID_STRUCT_MEMBER);
}

TEST(NameToPathVisitorTests, NameAndIds) {
  RootThriftPath<TestStruct> testStructRoot;

  // both resolve to the same path
  std::vector<std::vector<std::string>> paths = {
      {"member", "min"},
      {"4", "1"},
  };
  for (auto& path : paths) {
    auto result = RootNameToPathVisitor<RootThriftPath<TestStruct>>::visit(
        testStructRoot,
        path.begin(),
        path.begin(),
        path.end(),
        [&](auto resolved) {
          EXPECT_EQ(
              resolved.tokens(), std::vector<std::string>({"member", "min"}));
          EXPECT_EQ(resolved.idTokens(), std::vector<std::string>({"4", "1"}));
        });
    EXPECT_EQ(result, NameToPathResult::OK)
        << "Failed path: /" + folly::join('/', path);
  }
}

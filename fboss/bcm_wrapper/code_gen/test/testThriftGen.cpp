/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <clang/Tooling/Tooling.h>
#include <gtest/gtest.h>

#include "fboss/bcm_wrapper/code_gen/HeaderParser.h"

class ThriftGenTest : public ::testing::Test {
 public:
  void SetUp() override {
    mf_.addMatcher(facebook::fboss::HeaderParser::recordDeclMatcher(), &hp_);
    mf_.addMatcher(facebook::fboss::HeaderParser::enumDeclMatcher(), &hp_);
    mf_.addMatcher(facebook::fboss::HeaderParser::functionDeclMatcher(), &hp_);
  }
  // given some c source code, generate the corresponding thrift
  std::string genThrift(const std::string& source) {
    clang::tooling::runToolOnCode(
        clang::tooling::newFrontendActionFactory(&mf_).get()->create(),
        source,
        "lol.h");
    return hp_.getThrift();
  }
 private:
  facebook::fboss::HeaderParser hp_;
  clang::ast_matchers::MatchFinder mf_;
};

/*
 * Test the simplest struct: a struct with one primitive field
 */
TEST_F(ThriftGenTest, SimpleStruct) {
  const auto source = "struct foo { int x; };";
  const auto expected = R"(struct Foo
{
    0: i32 x
}
service BcmWrapperService
{
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we support the common pattern of typedef-ing struct foo_s to foo_t
 */
TEST_F(ThriftGenTest, TypedefStruct) {
  const auto source = "typedef struct foo_s { int x; }; foo_t";
  const auto expected = R"(struct Foo
{
    0: i32 x
}
service BcmWrapperService
{
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we properly handle c structs whose fields are other c structs
 */
TEST_F(ThriftGenTest, NestedStruct) {
  const auto source =
      "struct foo { int x; }; struct bar { int x; struct foo f; };";
  const auto expected = R"(struct Foo
{
    0: i32 x
}
struct Bar
{
    0: i32 x
    1: Foo f
}
service BcmWrapperService
{
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we properly handle simple enums that explicitly initialize
 * their enumerators
 */
TEST_F(ThriftGenTest, Enum) {
  auto source = "enum Foo { LOL = 0, HAHA = 1 };";
  const auto expected = R"(enum Foo
{
    LOL = 0,
    HAHA = 1,
}
service BcmWrapperService
{
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we properly handle a struct that has an enum field
 */
TEST_F(ThriftGenTest, EnumInStruct) {
  auto source = "enum foo { LOL = 0, HAHA = 1 }; struct bar { enum foo f; };";
  const auto expected = R"(enum Foo
{
    LOL = 0,
    HAHA = 1,
}
struct Bar
{
    0: Foo f
}
service BcmWrapperService
{
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we properly handle a struct that has a pointer field
 */
TEST_F(ThriftGenTest, PointerInStruct) {
  const auto source = "struct foo { int* x; };";
  const auto expected = R"(struct Foo
{
    0: list<i32> x
}
service BcmWrapperService
{
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we don't create a thrift struct if the c struct has a function
 * pointer field
 */
TEST_F(ThriftGenTest, FunctionPointerInStruct) {
  const auto source = "struct foo { int (*f)(); };";
  const auto expected = R"(service BcmWrapperService
{
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we properly handle a function with a return value but no pointer
 * parameters
 */
TEST_F(ThriftGenTest, SimpleFunction) {
  auto source = "int foo(int x);";
  auto expected = R"(struct FooResult
{
    0: i32 retVal
}
service BcmWrapperService
{
    FooResult foo(0: i32 x)
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we properly handle a function with a return value and pointer
 * parameters
 */
TEST_F(ThriftGenTest, PointerParameterFunction) {
  auto source = "int foo(int *x);";
  auto expected = R"(struct FooResult
{
    0: i32 retVal
    1: list<i32> x
}
service BcmWrapperService
{
    FooResult foo(0: list<i32> x)
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we properly handle a function with no return value or pointer
 * parameters
 */
TEST_F(ThriftGenTest, VoidReturnValueFunction) {
  auto source = "void foo(int x);";
  auto expected = R"(service BcmWrapperService
{
    void foo(0: i32 x)
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we properly handle a function with no return value but which has
 * pointer parameters
 */
TEST_F(ThriftGenTest, VoidReturnPointerParameterFunction) {
  auto source = "void foo(int *x);";
  auto expected = R"(struct FooResult
{
    0: list<i32> x
}
service BcmWrapperService
{
    FooResult foo(0: list<i32> x)
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

/*
 * Test that we don't generate a return struct or a method definition if a
 * declared function has a function pointer parameter
 */
TEST_F(ThriftGenTest, FunctionPointerParameterFunction) {
  auto source = "int foo(int (*fn)(int));";
  auto expected = R"(service BcmWrapperService
{
}
)";
  auto actual = genThrift(source);
  EXPECT_EQ(actual, expected);
}

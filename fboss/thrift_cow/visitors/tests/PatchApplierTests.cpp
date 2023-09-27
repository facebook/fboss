// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/dynamic.h>
#include <gtest/gtest.h>

#include <fboss/thrift_cow/gen-cpp2/patch_types.h>
#include <fboss/thrift_cow/visitors/PatchApplier.h>
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_types.h"

using k = facebook::fboss::test_tags::strings;

namespace {

using folly::dynamic;
using namespace facebook::fboss;
using namespace facebook::fboss::thrift_cow;

TestStruct createTestStruct() {
  dynamic testDyn = dynamic::object("inlineBool", true)("inlineInt", 54)(
      "inlineString",
      "testname")("optionalString", "bla")("inlineStruct", dynamic::object("min", 10)("max", 20))("inlineVariant", dynamic::object("inlineInt", 99))("mapOfEnumToStruct", dynamic::object("3", dynamic::object("min", 100)("max", 200)));

  return apache::thrift::from_dynamic<TestStruct>(
      testDyn, apache::thrift::dynamic_format::JSON_1);
}

} // namespace

namespace facebook::fboss::thrift_cow {

TEST(PatchVisitorTests, simple) {
  auto structA = createTestStruct();

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
}

} // namespace facebook::fboss::thrift_cow

// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/visitors/tests/VisitorTestUtils.h"
#include <folly/dynamic.h>
#include <thrift/lib/cpp2/reflection/folly_dynamic.h>

namespace facebook::fboss::thrift_cow {

TestStruct createSimpleTestStruct() {
  using folly::dynamic;
  dynamic testDyn = dynamic::object("inlineBool", true)("inlineInt", 54)(
      "inlineString",
      "testname")("optionalString", "bla")("inlineStruct", dynamic::object("min", 10)("max", 20))("inlineVariant", dynamic::object("inlineInt", 99))("mapOfEnumToStruct", dynamic::object("3", dynamic::object("min", 100)("max", 200)));

  return apache::thrift::from_dynamic<TestStruct>(
      testDyn, apache::thrift::dynamic_format::JSON_1);
}

} // namespace facebook::fboss::thrift_cow

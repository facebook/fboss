// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/visitors/tests/VisitorTestUtils.h"
#include <folly/json/dynamic.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>

namespace facebook::fboss::thrift_cow {

TestStruct createSimpleTestStruct() {
  using folly::dynamic;
  dynamic testDyn = dynamic::object("inlineBool", true)("inlineInt", 54)(
      "inlineString", "testname")("optionalString", "bla")(
      "inlineStruct", dynamic::object("min", 10)("max", 20))(
      "inlineVariant", dynamic::object("inlineInt", 99))(
      "mapOfEnumToStruct",
      dynamic::object("3", dynamic::object("min", 100)("max", 200)))(
      "mapOfI32ToI32", dynamic::object(1, 1))(
      "cowMap", dynamic::object(1, true));

  return facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
}

TestStruct createHybridMapTestStruct() {
  using folly::dynamic;
  dynamic testDyn = dynamic::object("inlineBool", true)("inlineInt", 54)(
      "inlineString", "testname")("optionalString", "bla")(
      "inlineStruct", dynamic::object("min", 10)("max", 20))(
      "inlineVariant", dynamic::object("inlineInt", 99))(
      "mapOfEnumToStruct",
      dynamic::object("3", dynamic::object("min", 100)("max", 200)))(
      "mapOfI32ToI32", dynamic::object(1, 1))(
      "cowMap", dynamic::object(1, true))(
      "hybridMap", dynamic::object(1, true));

  return facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
}

} // namespace facebook::fboss::thrift_cow
